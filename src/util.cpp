#include <algorithm>
#include <string.h>
#include <string>
#include <vector>
#include <cmath>
#include "util.h"
#include "rapidhash.h"
#include "context.h"

namespace hui
{

// evaluate a cubic hermite segment, m0/m1 are the forward derivatives at p0/p1
static Point cubicHermite(const Point& p0, const Point& m0, const Point& m1, const Point& p1, f32 t)
{
	f32 const t2 = t * t;
	f32 const t3 = t2 * t;
	f32 const h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
	f32 const h10 = t3 - 2.0f * t2 + t;
	f32 const h01 = -2.0f * t3 + 3.0f * t2;
	f32 const h11 = t3 - t2;

	return p0 * h00 + m0 * h10 + p1 * h01 + m1 * h11;
}

// clamp two tangents to prevent cubic hermite/bezier segments from developing
// loops or cusps. three-stage approach:
//   1. strip any backward (anti-chord) component - always causes a cusp
//   2. clamp each tangent magnitude to chord length - a large perpendicular
//      remainder after step 1 produces a huge sideways bow
//   3. clamp combined forward reach to chord length - prevents along-chord
//      components from overshooting and looping back
static void clampTangentsToChord(const Point& p0, const Point& p1, Point& m0, Point& m1)
{
	f32 const chordLen = (p1 - p0).getLength();

	if (chordLen < 1e-6f)
	{
		m0 = Point{ 0.0f, 0.0f };
		m1 = Point{ 0.0f, 0.0f };
		return;
	}

	Point const chordDir = (p1 - p0) / chordLen;

	// stage 1: remove any backward component
	f32 along0 = m0.dot(chordDir);
	f32 along1 = m1.dot(chordDir);

	if (along0 < 0.0f)
	{
		m0 = m0 - chordDir * along0;
		along0 = 0.0f;
	}

	if (along1 < 0.0f)
	{
		m1 = m1 - chordDir * along1;
		along1 = 0.0f;
	}

	// stage 2: per-tangent magnitude clamp to chord length so a large
	// perpendicular component cannot produce an extreme sideways bow
	f32 const len0 = m0.getLength();
	f32 const len1 = m1.getLength();

	if (len0 > chordLen)
	{
		f32 const s = chordLen / len0;
		m0 = m0 * s;
		along0 *= s;
	}

	if (len1 > chordLen)
	{
		f32 const s = chordLen / len1;
		m1 = m1 * s;
		along1 *= s;
	}

	// stage 3: clamp combined forward reach to chord length
	f32 const total = along0 + along1;

	if (total > chordLen)
	{
		f32 const scale = chordLen / total;
		m0 = m0 * scale;
		m1 = m1 * scale;
	}
}

// centripetal Catmull-Rom tangents (alpha = 0.5), avoids cusps and loops
// when the control points are unevenly spaced
static void centripetalCatmullRomTangents(const Point& p0, const Point& p1,
	const Point& prev, const Point& next, bool hasPrev, bool hasNext, Point& m0, Point& m1)
{
	f32 const l12 = sqrtf((p1 - p0).getLength());

	m0 = Point{ 0.0f, 0.0f };
	m1 = Point{ 0.0f, 0.0f };

	if (hasPrev && l12 > 1e-6f)
	{
		f32 const l01 = sqrtf((p0 - prev).getLength());

		if (l01 > 1e-6f)
		{
			f32 const a = 2.0f * l01 * l01 + 3.0f * l01 * l12 + l12 * l12;
			Point const c0 = (p0 * a - prev * (l12 * l12) + p1 * (l01 * l01)) / (3.0f * l01 * (l01 + l12));

			m0 = (c0 - p0) * 3.0f;
		}
	}

	if (hasNext && l12 > 1e-6f)
	{
		f32 const l23 = sqrtf((next - p1).getLength());

		if (l23 > 1e-6f)
		{
			f32 const b = 2.0f * l23 * l23 + 3.0f * l23 * l12 + l12 * l12;
			Point const c1 = (p1 * b + p0 * (l23 * l23) - next * (l12 * l12)) / (3.0f * l23 * (l23 + l12));

			m1 = (p1 - c1) * 3.0f;
		}
	}
}

// find the intersection of two lines, returns false when they are parallel
static bool lineIntersection(const Point& p0, const Point& d0, const Point& p1, const Point& d1, Point& out)
{
	f32 const denom = d0.x * d1.y - d0.y * d1.x;

	if (fabsf(denom) < 1e-6f)
		return false;

	f32 const t = ((p1.x - p0.x) * d1.y - (p1.y - p0.y) * d1.x) / denom;
	out = p0 + d0 * t;

	return true;
}

Point splineQuadraticControl(const SplineControlPoint& start, const SplineControlPoint& end)
{
	Point c;

	if (!lineIntersection(start.center, start.rightTangent, end.center, -end.leftTangent, c))
		c = ((start.center + start.rightTangent) + (end.center + end.leftTangent)) * 0.5f;

	return c;
}

// solve the tridiagonal system a[i] * x[i - 1] + b[i] * x[i] + c[i] * x[i + 1] = r[i]
// with the Thomas algorithm (a[0] and c[count - 1] are ignored)
static void tridiagPoints(const std::vector<f32>& a, const std::vector<f32>& b, const std::vector<f32>& c,
	const std::vector<Point>& r, std::vector<Point>& x)
{
	u32 const n = (u32)b.size();
	std::vector<f32> gam(n, 0.0f);
	f32 bet = b[0];

	if (fabsf(bet) < 1e-20f)
		bet = 1e-20f;

	x[0] = r[0] / bet;

	for (u32 j = 1; j < n; j++)
	{
		gam[j] = c[j - 1] / bet;
		bet = b[j] - a[j] * gam[j];

		if (fabsf(bet) < 1e-20f)
			bet = 1e-20f;

		x[j] = (r[j] - x[j - 1] * a[j]) / bet;
	}

	for (i32 j = (i32)n - 2; j >= 0; j--)
		x[j] = x[j] - x[j + 1] * gam[j + 1];
}

// build the second derivatives of a natural cubic spline by solving a
// tridiagonal system. when closed the control points form a loop and a
// periodic system is solved so the curve stays C2 across the seam
static std::vector<Point> computeNaturalSecondDerivatives(const SplineControlPoint* points, u32 count, bool closed)
{
	std::vector<Point> m(count, Point{ 0.0f, 0.0f });

	if (count < 3)
		return m;

	if (!closed)
	{
		u32 const n = count - 2;
		std::vector<f32> cPrime(n, 0.0f);
		std::vector<Point> dPrime(n);

		for (u32 k = 0; k < n; k++)
		{
			u32 const j = k + 1;
			Point const rhs = (points[j + 1].center - points[j].center * 2.0f + points[j - 1].center) * 6.0f;

			if (k == 0)
			{
				cPrime[k] = 1.0f / 4.0f;
				dPrime[k] = rhs / 4.0f;
			}
			else
			{
				f32 const denom = 4.0f - cPrime[k - 1];
				cPrime[k] = 1.0f / denom;
				dPrime[k] = (rhs - dPrime[k - 1]) / denom;
			}
		}

		m[count - 2] = dPrime[n - 1];

		for (i32 k = (i32)n - 2; k >= 0; k--)
			m[k + 1] = dPrime[k] - m[k + 2] * cPrime[k];

		return m;
	}

	// periodic system, all indices wrap around the loop
	u32 const n = count;
	std::vector<f32> a(n, 1.0f);
	std::vector<f32> b(n, 4.0f);
	std::vector<f32> c(n, 1.0f);
	std::vector<Point> r(n);

	for (u32 i = 0; i < n; i++)
	{
		Point const& pm = points[(i + n - 1) % n].center;
		Point const& p = points[i].center;
		Point const& pp = points[(i + 1) % n].center;
		r[i] = (pp - p * 2.0f + pm) * 6.0f;
	}

	// Sherman-Morrison correction for the wrap-around corners
	f32 const alpha = 1.0f;
	f32 const beta = 1.0f;
	f32 const gamma = -b[0];
	std::vector<f32> bb = b;
	bb[0] = b[0] - gamma;
	bb[n - 1] = b[n - 1] - alpha * beta / gamma;

	std::vector<Point> x(n), z(n), u(n, Point{ 0.0f, 0.0f });
	u[0] = Point{ gamma, gamma };
	u[n - 1] = Point{ alpha, alpha };

	tridiagPoints(a, bb, c, r, x);
	tridiagPoints(a, bb, c, u, z);

	f32 const denom = 1.0f + z[0].x + beta * z[n - 1].x / gamma;
	Point const fact = (x[0] + x[n - 1] * (beta / gamma)) / denom;

	for (u32 i = 0; i < n; i++)
		m[i] = x[i] - z[i] * fact;

	return m;
}

// evaluate a single spline segment at the parameter t in [0, 1]. when closed
// the segment from the last control point wraps around to the first one
static Point evaluateSplineSegment(const SplineControlPoint* points, u32 count, SplineType type,
	const Point* secondDerivs, u32 i, f32 t, bool closed)
{
	u32 const i0 = i;
	u32 const i1 = closed ? (i + 1) % count : (i + 1);
	Point const& p0 = points[i0].center;
	Point const& p1 = points[i1].center;

	switch (type)
	{
		case SplineType::CubicBezier:
		{
			// bezier tangents are handle offsets; clamp so b1/b2 can't cross
			// past each other and cause a loop
			Point m0 = points[i0].rightTangent;
			Point m1 = -points[i1].leftTangent;

			clampTangentsToChord(p0, p1, m0, m1);

			Point const b0 = p0;
			Point const b1 = p0 + m0;
			Point const b2 = p1 - m1;
			Point const b3 = p1;
			f32 const u = 1.0f - t;

			return b0 * (u * u * u) + b1 * (3.0f * u * u * t) + b2 * (3.0f * u * t * t) + b3 * (t * t * t);
		}
		case SplineType::QuadraticBezier:
		{
			Point const c = splineQuadraticControl(points[i0], points[i1]);
			f32 const u = 1.0f - t;

			return p0 * (u * u) + c * (2.0f * u * t) + p1 * (t * t);
		}
		case SplineType::CatmullRom:
		{
			bool const hasPrev = closed || i > 0;
			bool const hasNext = closed || (i + 2) < count;
			Point const prev = hasPrev ? points[(i0 + count - 1) % count].center : p0;
			Point const next = hasNext ? points[(i1 + 1) % count].center : p1;

			if (!hasPrev && !hasNext)
				return p0 + (p1 - p0) * t;

			Point m0, m1;

			centripetalCatmullRomTangents(p0, p1, prev, next, hasPrev, hasNext, m0, m1);

			return cubicHermite(p0, m0, m1, p1, t);
		}
		case SplineType::Natural:
		{
			Point const m0 = secondDerivs[i0];
			Point const m1 = secondDerivs[i1];
			f32 const u = 1.0f - t;

			return m0 * (u * u * u / 6.0f) + m1 * (t * t * t / 6.0f)
				+ (p0 - m0 / 6.0f) * u + (p1 - m1 / 6.0f) * t;
		}
		case SplineType::Linear:
			return p0 + (p1 - p0) * t;
		case SplineType::Basis:
		{
			// clamped uniform cubic B-spline, q0/q3 duplicate the end points so
			// the curve starts and ends on the first and last control point; a
			// closed curve wraps instead and becomes a periodic loop
			Point const q0 = (closed || i > 0) ? points[(i0 + count - 1) % count].center : p0;
			Point const q3 = (closed || (i + 2) < count) ? points[(i1 + 1) % count].center : p1;
			f32 const t2 = t * t;
			f32 const t3 = t2 * t;
			f32 const u = 1.0f - t;

			return (q0 * (u * u * u)
				+ p0 * (3.0f * t3 - 6.0f * t2 + 4.0f)
				+ p1 * (-3.0f * t3 + 3.0f * t2 + 3.0f * t + 1.0f)
				+ q3 * t3) / 6.0f;
		}
		case SplineType::Hermite:
		default:
		{
			Point m0 = points[i0].rightTangent;
			Point m1 = -points[i1].leftTangent;

			clampTangentsToChord(p0, p1, m0, m1);

			return cubicHermite(p0, m0, m1, p1, t);
		}
	}
}

// append the samples of one spline segment to out, the end point of the
// segment itself is left out so consecutive segments join without duplicates
static void sampleSplineSegment(const SplineControlPoint* points, u32 count, SplineType type,
	const Point* secondDerivs, u32 i, f32 segmentSize, bool closed, std::vector<Point>& out)
{
	u32 const coarseSteps = 16;
	f32 length = 0.0f;
	Point prev = evaluateSplineSegment(points, count, type, secondDerivs, i, 0.0f, closed);

	for (u32 k = 1; k <= coarseSteps; k++)
	{
		Point cur = evaluateSplineSegment(points, count, type, secondDerivs, i, (f32)k / (f32)coarseSteps, closed);
		length += (cur - prev).getLength();
		prev = cur;
	}

	u32 steps = (u32)ceilf(length / segmentSize);

	if (steps < 2)
		steps = 2;

	for (u32 k = 0; k < steps; k++)
		out.push_back(evaluateSplineSegment(points, count, type, secondDerivs, i, (f32)k / (f32)steps, closed));
}

u32 splineSample(const SplineControlPoint* points, u32 count, SplineType type, f32 segmentSize, bool closed, Point* outPoints, u32 maxPoints)
{
	if (!points || count < 2)
		return 0;

	std::vector<Point> secondDerivs;

	if (type == SplineType::Natural)
		secondDerivs = computeNaturalSecondDerivatives(points, count, closed);

	Point const* secondDerivsPtr = secondDerivs.empty() ? nullptr : secondDerivs.data();

	std::vector<Point> tmp;

	if (type == SplineType::Basis && !closed)
		tmp.push_back(points[0].center);

	u32 const segmentCount = closed ? count : count - 1;

	for (u32 i = 0; i < segmentCount; i++)
		sampleSplineSegment(points, count, type, secondDerivsPtr, i, segmentSize, closed, tmp);

	if (!closed)
		tmp.push_back(points[count - 1].center);

	if (!outPoints)
		return (u32)tmp.size();

	u32 n = (u32)tmp.size();

	if (n > maxPoints)
		n = maxPoints;

	memcpy(outPoints, tmp.data(), n * sizeof(Point));

	return n;
}

u32 splineSampleSegment(const SplineControlPoint* points, u32 count, SplineType type, i32 segment, f32 segmentSize, bool closed, Point* outPoints, u32 maxPoints)
{
	if (!points || count < 2 || segment < 0)
		return 0;

	u32 const segmentCount = closed ? count : count - 1;

	if ((u32)segment >= segmentCount)
		return 0;

	std::vector<Point> secondDerivs;

	if (type == SplineType::Natural)
		secondDerivs = computeNaturalSecondDerivatives(points, count, closed);

	Point const* secondDerivsPtr = secondDerivs.empty() ? nullptr : secondDerivs.data();

	std::vector<Point> tmp;

	if (type == SplineType::Basis && !closed && segment == 0)
		tmp.push_back(points[0].center);

	sampleSplineSegment(points, count, type, secondDerivsPtr, (u32)segment, segmentSize, closed, tmp);

	// finish the segment with its end point
	tmp.push_back(evaluateSplineSegment(points, count, type, secondDerivsPtr, (u32)segment, 1.0f, closed));

	if (!outPoints)
		return (u32)tmp.size();

	u32 n = (u32)tmp.size();

	if (n > maxPoints)
		n = maxPoints;

	memcpy(outPoints, tmp.data(), n * sizeof(Point));

	return n;
}

i32 splineClosestSegment(const SplineControlPoint* points, u32 count, SplineType type, const Point& query, Point* outClosestPoint, bool closed, f32* outT)
{
	if (!points || count < 2)
		return -1;

	std::vector<Point> secondDerivs;

	if (type == SplineType::Natural)
		secondDerivs = computeNaturalSecondDerivatives(points, count, closed);

	Point const* secondDerivsPtr = secondDerivs.empty() ? nullptr : secondDerivs.data();

	i32 bestSegment = -1;
	f32 bestDist = FLT_MAX;
	Point bestPoint = query;
	f32 bestT = 0.0f;

	u32 const segmentCount = closed ? count : count - 1;

	for (u32 i = 0; i < segmentCount; i++)
	{
		u32 const steps = 24;
		Point prev = evaluateSplineSegment(points, count, type, secondDerivsPtr, i, 0.0f, closed);

		for (u32 k = 1; k <= steps; k++)
		{
			Point cur = evaluateSplineSegment(points, count, type, secondDerivsPtr, i, (f32)k / (f32)steps, closed);
			Point ab = cur - prev;
			f32 len2 = ab.dot(ab);
			f32 t = len2 > 1e-6f ? (query - prev).dot(ab) / len2 : 0.0f;

			t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);

			Point proj = prev + ab * t;
			f32 dist = (query - proj).getLength();

			if (dist < bestDist)
			{
				bestDist = dist;
				bestSegment = (i32)i;
				bestPoint = proj;
				bestT = ((f32)(k - 1) + t) / (f32)steps;
			}

			prev = cur;
		}
	}

	if (outClosestPoint)
		*outClosestPoint = bestPoint;

	if (outT)
		*outT = bestT;

	return bestSegment;
}

WidgetId genId(const char* text)
{
	return hashString(text, ctx->idStack.empty() ? 0 : ctx->idStack.back());
}

WidgetId genId(u32 id)
{
	return hashData(&id, sizeof(id), ctx->idStack.empty() ? 0 : ctx->idStack.back());
}

WidgetId genId(void* ptr)
{
	return hashData(&ptr, sizeof(ptr), ctx->idStack.empty() ? 0 : ctx->idStack.back());
}

WidgetId genIdFromPosition(const char* text)
{
	auto posStr = std::to_string(ctx->position.x) + std::to_string(ctx->position.y);

	return genId((std::string(text) + posStr).c_str());
}

void stringFromI32(i32 value, char* outString, u32 outStringMaxSize, u32 fillerZeroesCount)
{
	if (fillerZeroesCount)
	{
		std::string zeroCountStr;
		zeroCountStr = std::to_string(fillerZeroesCount);
		std::string strFmt = std::string("%.") + zeroCountStr;
		strFmt += "d";
		snprintf(outString, outStringMaxSize, strFmt.c_str(), value);
	}
	else
		snprintf(outString, outStringMaxSize, "%d", value);
}

void stringFromF32(f32 value, char* outString, u32 outStringMaxSize, i32 decimalPlaces)
{
	if (decimalPlaces >= 0)
	{
		std::string decimalsStr;
		decimalsStr = std::to_string(decimalPlaces);
		std::string strFmt = std::string("%.") + decimalsStr;
		strFmt += "f";
		snprintf(outString, outStringMaxSize, strFmt.c_str(), value);
	}
	else
		snprintf(outString, outStringMaxSize, "%f", value);
}

void toHexString(int n, char* outString, u32 outStringMaxSize, bool lowercase = true)
{
	if (lowercase)
		snprintf(outString, outStringMaxSize, "%02x", n);
	else
		snprintf(outString, outStringMaxSize, "%02X", n);
}

bool clampValue(f32& value, f32 minVal, f32 maxVal)
{
	if (value < minVal)
	{
		value = minVal;
		return true;
	}

	if (value > maxVal)
	{
		value = maxVal;
		return true;
	}

	return false;
}

u64 hashString(const char* str, u64 seed)
{
	return rapidhash_withSeed_unrolled(str, strlen(str), seed ? seed : RAPID_SEED);
}

u64 hashData(const void* ptr, size_t size, u64 seed)
{
	return rapidhash_withSeed_unrolled(ptr, size, seed ? seed : RAPID_SEED);
}

}
