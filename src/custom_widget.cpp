#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "util.h"
#include "font.h"
#include "context.h"
#define _USE_MATH_DEFINES
#include <math.h>

namespace hui
{
Rect beginCustomWidget(f32 height)
{
	if (height <= 0)
	{
		height = ctx->layoutStack.back().height - (ctx->penPosition.y - ctx->layoutStack.back().position.y);
	}

	addWidgetItem(height);
	buttonBehavior();
	ctx->currentWidgetId++;

	return ctx->widget.rect;
}

void endCustomWidget()
{
	auto wnd = ctx->providers->input->getCurrentWindow();
	auto rc = ctx->providers->input->getWindowRect(wnd);
}

Point getParentSize()
{
	Point pt;

	pt.x = ctx->layoutStack.back().width;
	pt.y = ctx->layoutStack.back().height;

	return pt;
}

Rect getWidgetRect()
{
	return { ctx->widget.rect.x, ctx->widget.rect.y, ctx->widget.rect.width, ctx->widget.rect.height };
}

void pushDrawCommandIndex()
{
	ctx->drawCmdIndexStack.push_back(ctx->renderer->getDrawCommandCount());
}

u32 popDrawCommandIndex()
{
	if (ctx->drawCmdIndexStack.size())
	{
		auto idx = ctx->drawCmdIndexStack.back();
		ctx->drawCmdIndexStack.pop_back();
		return idx;
	}

	return 0;
}

void beginInsertDrawCommands(u32 index)
{
	ctx->renderer->beginDrawCmdInsertion(index);
}

void endInsertDrawCommands()
{
	ctx->renderer->endDrawCmdInsertion();
}

void setFont(HFont font)
{
	ctx->renderer->cmdSetFont((UiFont*)font);
}

void setColor(const Color& color)
{
	ctx->renderer->cmdSetColor(color);
}

void setLineColor(const Color& color)
{
	ctx->renderer->currentLineStyle.color = color;
	ctx->renderer->cmdSetLineStyle(ctx->renderer->currentLineStyle);
}

void setFillColor(const Color& color)
{
	ctx->renderer->currentFillStyle.color = color;
	ctx->renderer->cmdSetFillStyle(ctx->renderer->currentFillStyle);
}

Point getTextSize(const char* text)
{
	if (!ctx->renderer->getFont())
		return Point();

	auto fntInfo = ctx->renderer->getFont()->computeTextSize(*ctx->textCache->getText(text));

	return { fntInfo.width, fntInfo.height };
}

void drawTextAt(const char* text, const Point& position)
{
	ctx->renderer->cmdDrawTextAt(text, position + ctx->renderer->viewportOffset);
}

void drawTextInBox(const char* text, const Rect& rect, HAlignType horizontalAlign, VAlignType verticalAlign)
{
	ctx->renderer->cmdDrawTextInBox(
		text,
		Rect(
			rect.x + ctx->renderer->viewportOffset.x,
			rect.y + ctx->renderer->viewportOffset.y,
			rect.width, rect.height),
		horizontalAlign, verticalAlign);
}

void drawImage(HImage image, const Point& position, f32 scale)
{
	UiImage* img = (UiImage*)image;
	ctx->renderer->cmdDrawImage(img, position + ctx->renderer->viewportOffset, scale);
}

void drawStretchedImage(HImage image, const Rect& rect)
{
	UiImage* img = (UiImage*)image;
	ctx->renderer->cmdDrawImage(img, Rect(rect.x + ctx->renderer->viewportOffset.x, rect.y + ctx->renderer->viewportOffset.y, rect.width, rect.height));
}

void drawBorderedImage(HImage image, u32 border, const Rect& rect)
{
	UiImage* img = (UiImage*)image;

	ctx->renderer->cmdDrawImageBordered(img, border, Rect(rect.x + ctx->renderer->viewportOffset.x, rect.y + ctx->renderer->viewportOffset.y, rect.width, rect.height), ctx->globalScale);
}

void setLineStyle(const LineStyle& style)
{
	ctx->renderer->cmdSetLineStyle(style);
}

void setFillStyle(const FillStyle& style)
{
	ctx->fillStyle = style;
	ctx->renderer->cmdSetColor(style.color);
}

void drawLine(const Point& a, const Point& b)
{
	ctx->renderer->cmdDrawLine(a + ctx->renderer->viewportOffset, b + ctx->renderer->viewportOffset);
}

void drawPolyLine(const Point* points, u32 pointCount, bool closed)
{
	std::vector<Point> pts;
	
	pts.resize(pointCount);

	for (u32 i = 0; i < pointCount; i++)
	{
		pts[i] = points[i] + ctx->renderer->viewportOffset;
	}

	ctx->renderer->cmdDrawPolyLine(pts.data(), pointCount, closed);
}

void drawCircle(const Point& center, f32 radius, u32 segments)
{
	drawEllipse(center, radius, radius, segments);
}

void drawEllipse(const Point& center, f32 radiusX, f32 radiusY, u32 segments)
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
		pts.push_back(pt + ctx->renderer->viewportOffset);
		crtAngle += step;
	}

	ctx->renderer->cmdDrawPolyLine(pts.data(), pts.size(), true);
}

void drawRectangle(const Rect& rc)
{
	Point pts[4] = {
		rc.topLeft() + ctx->renderer->viewportOffset,
		rc.topRight() + ctx->renderer->viewportOffset,
		rc.bottomRight() + ctx->renderer->viewportOffset,
		rc.bottomLeft() + ctx->renderer->viewportOffset
	};

	ctx->renderer->cmdDrawPolyLine(pts, 4, true);
}

void drawSolidRectangle(const Rect& rc)
{
	ctx->renderer->cmdDrawSolidRectangle(
		{
			ctx->renderer->viewportOffset.x + rc.x,
			ctx->renderer->viewportOffset.y + rc.y,
			rc.width,
			rc.height
		});
}

#define HORUS_HERMITE_TANGENT(a, b, c, tt, cc, bb, adj)\
		(((b - a) * (1.0f + bb) * (1.0f - cc) + (c - b)\
		* (1.0f - bb) * (1.0f + cc)) * (1.0f - tt) * adj)

#define HORUS_HERMITE_ONE_TANGENT(a, b, tt) ((a - b) * (1.0f - tt))

#define HORUS_HERMITE_FIRST_TANGENT(a, b, c, tt) (((a - b) * 1.5f - c * 0.5f) * (1.0f - tt))

Point hermitePoint(
	const Point& controlPtA,
	const Point& tangentA,
	const Point& tangentB,
	const Point& controlPtB,
	f32 time)
{
	f32 t1, t2, t3, t4, tp3, tp2;
	Point v;

	tp3 = time * time * time;
	tp2 = time * time;

	t1 = 2.0f * tp3 - 3.0f * tp2 + 1.0f;
	t2 = tp3 - 2.0f * tp2 + time;
	t3 = (-2.0f * tp3) + 3.0f * tp2;
	t4 = tp3 - tp2;

	v.x = controlPtA.x * t1 + tangentA.x * t2 + controlPtB.x * t3 - tangentB.x * t4;
	v.y = controlPtA.y * t1 + tangentA.y * t2 + controlPtB.y * t3 - tangentB.y * t4;

	return v;
}

// Compute a length of a spline segment by using 5-point Legendre-Gauss quadrature
// https://en.wikipedia.org/wiki/Gaussian_quadrature
f32 computeSplineLength(const Point& start, const Point& start_tangent,
	const Point& end, Point const& end_tangent)
{
	// Cubic Hermite spline derivative coefficients
	Point const c0 = start_tangent;
	Point const c1 = (end - start) * 6.0f - start_tangent * 4.0f - end_tangent * 2.0f;
	Point const c2 = (start - end) * 6.0f + (start_tangent + end_tangent) * 3.0f;
	
	auto const evaluate_derivative = [c0, c1, c2](float t) -> Point
	{
		return c0 + (c1 + c2 * t) * t;
	};

	struct GaussLengendreCoefficient
	{
		float abscissa;
		float weight;
	};

	static constexpr GaussLengendreCoefficient gauss_lengendre_coefficients[] =
	{
		{ 0.0f, 0.5688889f },
		{ -0.5384693f, 0.47862867f },
		{ 0.5384693f, 0.47862867f },
		{ -0.90617985f, 0.23692688f },
		{ 0.90617985f, 0.23692688f }
	};
	
	float length = 0.f;
	
	for (auto coefficient : gauss_lengendre_coefficients)
	{
		float const t = 0.5f * (1.f + coefficient.abscissa);
		length += evaluate_derivative(t).getLength() * coefficient.weight;
	}

	return 0.5f * length;
}

void drawSpline(SplineControlPoint* points, u32 count, f32 segmentSize)
{
	std::vector<Point> pts;

	for (int i = 0; i < count - 1; i++)
	{
		f32 len = computeSplineLength(points[i].center, points[i].rightTangent, points[i + 1].center, points[i + 1].leftTangent);
		f32 step = 1.0f / (len / segmentSize);

		for (f32 s = 0; s < 1.0f; s += step)
		{
			pts.push_back(
				hermitePoint(points[i].center, points[i].rightTangent, points[i + 1].leftTangent, points[i + 1].center, s));
		}
	}

	drawPolyLine(pts.data(), pts.size());
}

void drawArrow(const Point& a, const Point& b, f32 tipLength, f32 tipWidth, bool drawBodyLine)
{
	//TODO
}

void drawSolidTriangle(
	const Point& p1, const Point& p2, const Point& p3)
{
	ctx->renderer->cmdDrawSolidTriangle(
		p1 + ctx->renderer->viewportOffset,
		p2 + ctx->renderer->viewportOffset,
		p3 + ctx->renderer->viewportOffset);
}

}