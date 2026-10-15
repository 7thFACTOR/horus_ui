#include "context.h"
#include "renderer.h"
#include "theme.h"
#include "util.h"
#include "font.h"
#define _USE_MATH_DEFINES
#include <math.h>

namespace hui
{
Rect customWidgetBegin(const char* id, f32 height)
{
	if (height <= 0)
	{
		height = ctx->layout.height - (ctx->position.y - ctx->layout.savedPosition.y);
	}

	ctx->id = genId(id);
	addWidget(height);
	buttonBehavior();

	return ctx->widget.rect;
}

void customWidgetEnd()
{
}

void rendererFontSet(HFont font)
{
	ctx->renderer.cmdSetFont((Font*)font);
}

void rendererFontPush(HFont font)
{
	ctx->fontStack.push_back((HFont)ctx->renderer.getFont());
	rendererFontSet(font);
}

void rendererFontPop()
{
	if (ctx->fontStack.size())
	{
		rendererFontSet(ctx->fontStack.back());
		ctx->fontStack.pop_back();
	}
}

void rendererColorSet(const Color& color)
{
	ctx->renderer.cmdSetColor(color);
}

void rendererLineColorSet(const Color& color)
{
	ctx->renderer.currentLineStyle.color = color;
	ctx->renderer.cmdSetLineStyle(ctx->renderer.currentLineStyle);
}

void rendererFillColorSet(const Color& color)
{
	ctx->renderer.currentFillStyle.color = color;
	ctx->renderer.cmdSetFillStyle(ctx->renderer.currentFillStyle);
}

Point renderGetTextSize(const char* text)
{
	if (!ctx->renderer.getFont())
		return Point();

	FontTextSize fntInfo = 
	ctx->renderer.computeSizeOrDrawText(text, Rect(0, 0, FLT_MAX, FLT_MAX), HAlignType::Left, VAlignType::Top, false, ctx->renderer.getFont());

	return { fntInfo.width, fntInfo.height };
}

void rendererTextAtDraw(const char* text, const Point& position)
{
	ctx->renderer.cmdDrawTextAt(text, position);
}

void renderDrawTextInBox(const char* text, const Rect& rect, HAlignType horizontalAlign, VAlignType verticalAlign)
{
	ctx->renderer.cmdDrawTextInBox(text, rect, horizontalAlign, verticalAlign);
}

void renderDrawImage(HImage image, const Point& position, f32 scale)
{
	Image* img = (Image*)image;
	ctx->renderer.cmdDrawImage(img, position, scale);
}

void renderDrawStretchedImage(HImage image, const Rect& rect)
{
	Image* img = (Image*)image;
	ctx->renderer.cmdDrawImage(img, rect);
}

void renderDrawBorderedImage(HImage image, u32 border, const Rect& rect)
{
	Image* img = (Image*)image;

	ctx->renderer.cmdDrawImageBordered(img, border, rect, ctx->scale);
}

void renderSetLineStyle(const LineStyle& style)
{
	ctx->renderer.cmdSetLineStyle(style);
}

void renderSetFillStyle(const FillStyle& style)
{
	ctx->fillStyle = style;
	ctx->renderer.cmdSetColor(style.color);
}

void renderSetColor(const Color& color)
{
	ctx->renderer.cmdSetColor(color);
}

void renderDrawLine(const Point& a, const Point& b)
{
	ctx->renderer.cmdDrawLine(a, b);
}

void renderDrawPolyLine(const Point* points, u32 pointCount, bool closed)
{
	ctx->renderer.cmdDrawPolyLine(points, pointCount, closed);
}

void renderDrawCircle(const Point& center, f32 radius, u32 segments)
{
	renderDrawEllipse(center, radius, radius, segments);
}

void renderDrawEllipse(const Point& center, f32 radiusX, f32 radiusY, u32 segments)
{
	std::vector<Point> pts;
	Point pt;
	f32 crtAngle = 0, step;

	pts.reserve(segments);
	step = 2 * M_PI / (f32)segments;

	for (u32 i = 0; i < segments; i++)
	{
		pt.x = center.x + radiusX * sinf(crtAngle);
		pt.y = center.y + radiusY * cosf(crtAngle);
		pts.push_back(pt);
		crtAngle += step;
	}

	ctx->renderer.cmdDrawPolyLine(pts.data(), pts.size(), true);
}

// enqueue a filled circle sector as a triangle fan around the center
static void drawSolidSector(const Point& center, f32 radius, f32 startAngle, f32 endAngle, u32 segments)
{
	if (segments < 1)
		segments = 1;

	Rgba32 const color = ctx->renderer.currentColor;
	f32 const step = (endAngle - startAngle) / (f32)segments;
	Point prev, cur;

	prev.x = center.x + cosf(startAngle) * radius;
	prev.y = center.y + sinf(startAngle) * radius;

	for (u32 i = 1; i <= segments; i++)
	{
		f32 const angle = startAngle + step * (f32)i;

		cur.x = center.x + cosf(angle) * radius;
		cur.y = center.y + sinf(angle) * radius;

		ctx->renderer.cmdDrawSolidTriangle(center, prev, cur, color, color, color);
		prev = cur;
	}

	// anti-aliasing: fade out a thin skirt just outside the boundary using the
	// per-vertex alpha of the white image triangles
	if (!ctx->settings.lineAntiAliasing)
		return;

	f32 const feather = ctx->settings.lineAntiAliasingSkirtWidthPixels;

	if (feather <= 0.0f)
		return;

	Rgba32 const colorFade = color & 0x00ffffffu;
	f32 const outerRadius = radius + feather;
	f32 const fullSweep = 2.0f * (f32)M_PI;

	// a pie has two straight radial edges, fade them outwards too
	if (fabsf(endAngle - startAngle) < fullSweep - 1e-4f)
	{
		Point const dStart = { cosf(startAngle), sinf(startAngle) };
		Point const nStart = { sinf(startAngle), -cosf(startAngle) };
		Point const a0 = center;
		Point const a1 = center + dStart * radius;
		Point const a0o = center + nStart * feather;
		Point const a1o = a1 + nStart * feather;

		ctx->renderer.cmdDrawSolidTriangle(a0, a1, a1o, color, color, colorFade);
		ctx->renderer.cmdDrawSolidTriangle(a0, a1o, a0o, color, colorFade, colorFade);

		Point const dEnd = { cosf(endAngle), sinf(endAngle) };
		Point const nEnd = { -sinf(endAngle), cosf(endAngle) };
		Point const b0 = center;
		Point const b1 = center + dEnd * radius;
		Point const b0o = center + nEnd * feather;
		Point const b1o = b1 + nEnd * feather;

		ctx->renderer.cmdDrawSolidTriangle(b0, b1, b1o, color, color, colorFade);
		ctx->renderer.cmdDrawSolidTriangle(b0, b1o, b0o, color, colorFade, colorFade);
	}

	// fade the curved boundary outwards
	Point innerPrev, outerPrev;

	innerPrev.x = center.x + cosf(startAngle) * radius;
	innerPrev.y = center.y + sinf(startAngle) * radius;
	outerPrev.x = center.x + cosf(startAngle) * outerRadius;
	outerPrev.y = center.y + sinf(startAngle) * outerRadius;

	for (u32 i = 1; i <= segments; i++)
	{
		f32 const angle = startAngle + step * (f32)i;
		Point innerCur, outerCur;

		innerCur.x = center.x + cosf(angle) * radius;
		innerCur.y = center.y + sinf(angle) * radius;
		outerCur.x = center.x + cosf(angle) * outerRadius;
		outerCur.y = center.y + sinf(angle) * outerRadius;

		ctx->renderer.cmdDrawSolidTriangle(innerPrev, innerCur, outerCur, color, color, colorFade);
		ctx->renderer.cmdDrawSolidTriangle(innerPrev, outerCur, outerPrev, color, colorFade, colorFade);

		innerPrev = innerCur;
		outerPrev = outerCur;
	}
}

void renderDrawSolidCircle(const Point& center, f32 radius, u32 segments)
{
	drawSolidSector(center, radius, 0.0f, 2.0f * M_PI, segments);
}

void renderDrawSolidPie(const Point& center, f32 radius, f32 startAngle, f32 endAngle, u32 segments)
{
	drawSolidSector(center, radius, startAngle, endAngle, segments);
}

void renderDrawPie(const Point& center, f32 radius, f32 startAngle, f32 endAngle, u32 segments)
{
	if (segments < 1)
		segments = 1;

	std::vector<Point> pts;
	f32 const step = (endAngle - startAngle) / (f32)segments;

	pts.reserve(segments + 2);
	pts.push_back(center);

	for (u32 i = 0; i <= segments; i++)
	{
		f32 const angle = startAngle + step * (f32)i;
		Point pt;

		pt.x = center.x + cosf(angle) * radius;
		pt.y = center.y + sinf(angle) * radius;

		pts.push_back(pt);
	}

	// close the outline back at the center so the two radii are drawn
	pts.push_back(center);

	ctx->renderer.cmdDrawPolyLine(pts.data(), (u32)pts.size(), false);
}

void renderDrawRectangle(const Rect& rc)
{
	Point pts[4] = {
		rc.topLeft(),
		rc.topRight(),
		rc.bottomRight(),
		rc.bottomLeft()
	};

	ctx->renderer.cmdDrawPolyLine(pts, 4, true);
}

void renderDrawSolidRectangle(const Rect& rc)
{
	ctx->renderer.cmdDrawFilledRectangle(rc);
}

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
			Point const b0 = p0;
			Point const b1 = p0 + points[i0].rightTangent;
			Point const b2 = p1 + points[i1].leftTangent;
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
			Point const m0 = points[i0].rightTangent;
			Point const m1 = -points[i1].leftTangent;

			return cubicHermite(p0, m0, m1, p1, t);
		}
	}
}

void renderDrawSpline(SplineControlPoint* points, u32 count, SplineType type, f32 segmentSize, bool closed)
{
	if (!points || count < 2)
		return;

	std::vector<Point> pts;
	std::vector<Point> secondDerivs;

	if (type == SplineType::Natural)
		secondDerivs = computeNaturalSecondDerivatives(points, count, closed);

	Point const* secondDerivsPtr = secondDerivs.empty() ? nullptr : secondDerivs.data();

	// the uniform B-spline does not interpolate the first control point, so
	// prepend it to keep the drawn curve starting on it (open curves only)
	if (type == SplineType::Basis && !closed)
		pts.push_back(points[0].center);

	u32 const segmentCount = closed ? count : count - 1;

	for (u32 i = 0; i < segmentCount; i++)
	{
		// estimate the segment length with a coarse sampling, then pick a
		// step count that keeps the resulting segments about segmentSize long
		u32 const coarseSteps = 16;
		f32 length = 0.0f;
		Point prev = evaluateSplineSegment(points, count, type, secondDerivsPtr, i, 0.0f, closed);

		for (u32 k = 1; k <= coarseSteps; k++)
		{
			Point cur = evaluateSplineSegment(points, count, type, secondDerivsPtr, i, (f32)k / (f32)coarseSteps, closed);
			length += (cur - prev).getLength();
			prev = cur;
		}

		u32 steps = (u32)ceilf(length / segmentSize);

		if (steps < 2)
			steps = 2;

		for (u32 k = 0; k < steps; k++)
			pts.push_back(evaluateSplineSegment(points, count, type, secondDerivsPtr, i, (f32)k / (f32)steps, closed));
	}

	if (!closed)
		pts.push_back(points[count - 1].center);

	renderDrawPolyLine(pts.data(), (u32)pts.size(), closed);
}

i32 splineClosestSegment(const SplineControlPoint* points, u32 count, SplineType type, const Point& query, Point* outClosestPoint, bool closed)
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

	u32 const segmentCount = closed ? count : count - 1;

	for (u32 i = 0; i < segmentCount; i++)
	{
		// sample the curve and project the query onto each little span
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
			}

			prev = cur;
		}
	}

	if (outClosestPoint)
		*outClosestPoint = bestPoint;

	return bestSegment;
}

void renderDrawArrow(const Point& a, const Point& b, f32 tipLength, f32 tipWidth, bool drawBodyLine)
{
	//TODO
}

void renderDrawSolidTriangle(
	const Point& p1, const Point& p2, const Point& p3)
{
	ctx->renderer.cmdDrawSolidTriangle(
		p1, p2, p3, ctx->renderer.currentColor, ctx->renderer.currentColor, ctx->renderer.currentColor);
}

}