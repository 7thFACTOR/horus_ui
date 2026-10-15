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

void renderDrawSpline(SplineControlPoint* points, u32 count, SplineType type, f32 segmentSize, bool closed)
{
	u32 n = splineSample(points, count, type, segmentSize, closed, nullptr, 0);

	if (n == 0)
		return;

	std::vector<Point> pts(n);
	splineSample(points, count, type, segmentSize, closed, pts.data(), n);

	renderDrawPolyLine(pts.data(), n, closed);
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