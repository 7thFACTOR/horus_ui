#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "ui_theme.h"
#include "text_cache.h"
#include "util.h"
#include "ui_font.h"
#include "ui_context.h"
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
	auto wnd = ctx->inputProvider->getCurrentWindow();
	auto rc = ctx->inputProvider->getWindowRect(wnd);
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

void setFont(Font font)
{
	ctx->renderer->cmdSetFont((UiFont*)font);
}

void setBackColor(const Color& color)
{
	ctx->renderer->cmdSetColor(color);
}

Point getTextSize(Utf8String text)
{
	if (!ctx->renderer->getFont())
		return Point();
	
	auto fntInfo = ctx->renderer->getFont()->computeTextSize(*ctx->textCache->getText(text));

	return { fntInfo.width, fntInfo.height };
}

void drawTextAt(Utf8String text, const Point& position)
{
	ctx->renderer->cmdDrawTextAt(text, Point(position.x, position.y));
}

void drawTextInBox(Utf8String text, const Rect& rect, HAlignType horizontalAlign, VAlignType verticalAlign)
{
	ctx->renderer->cmdDrawTextInBox(
		text,
		Rect(rect.x, rect.y, rect.width, rect.height),
		horizontalAlign, verticalAlign);
}

void drawImage(Image image, const Point& position)
{
	UiImage* img = (UiImage*)image;
	ctx->renderer->cmdDrawImage(img, Point(position.x, position.y));
}

void drawStretchedImage(Image image, const Rect& rect)
{
	UiImage* img = (UiImage*)image;
	ctx->renderer->cmdDrawImage(img, Rect(rect.x, rect.y, rect.width, rect.height));
}

void drawBorderedImage(Image image, u32 border, const Rect& rect)
{
	UiImage* img = (UiImage*)image;

	ctx->renderer->cmdDrawImageBordered(img, border, Rect(rect.x, rect.y, rect.width, rect.height), ctx->globalScale);
}

void setLineStyle(const LineStyle& style)
{
	ctx->renderer->cmdSetLineStyle(style);
}

void setFillStyle(const FillStyle& style)
{
	ctx->fillStyle = style;
}

void drawLine(const Point& a, const Point& b)
{
	ctx->renderer->cmdDrawLine(a, b);
}

void drawPolyLine(const Point* points, u32 pointCount, bool closed)
{
	ctx->renderer->cmdDrawPolyLine(points, pointCount, closed);
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
	segments++;

	for (u32 i = 0; i < segments; i++)
	{
		pt.x = center.x + radiusX * sinf(crtAngle);
		pt.y = center.y + radiusY * cosf(crtAngle);
		crtAngle += step;
		pts.push_back(pt);
	}

	ctx->renderer->cmdDrawPolyLine(pts.data(), pts.size(), true);
}

void drawRectangle(const Rect& rc)
{
	Point pts[4] = {
		rc.topLeft(),
		rc.topRight(),
		rc.bottomRight(),
		rc.bottomLeft()
	};
	ctx->renderer->cmdDrawPolyLine(pts, 4, true);
}

#define B_HERMITE_TANGENT(a, b, c, tt, cc, bb, adj)\
			(((b - a) * (1.0f + bb) * (1.0f - cc) + (c - b)\
			* (1.0f - bb) * (1.0f + cc)) * (1.0f - tt) * adj)

#define B_HERMITE_ONE_TANGENT(a, b, tt) ((a - b) * (1.0f - tt))

#define B_HERMITE_FIRST_TANGENT(a, b, c, tt) (((a - b) * 1.5f - c * 0.5f) * (1.0f - tt))

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

void drawSpline(SplineControlPoint* points, u32 count)
{
	std::vector<Point> pts;
	f32 step = 0.02f;

	for (int i = 0; i < count - 1; i++)
	{
		for (f32 s = 0; s < 1.0f; s += step)
		{
			pts.push_back(hermitePoint(points[i].center, points[i].rightTangent, points[i+1].leftTangent, points[i+1].center, s));
		}
	}

	drawPolyLine(pts.data(), pts.size());
}

void drawArrow(const Point& a, const Point& b, f32 tipLength, f32 tipWidth, bool drawBodyLine)
{
	//TODO
}

}