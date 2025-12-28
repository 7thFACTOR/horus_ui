#include "context.h"
#include "theme.h"
#include "util.h"

namespace hui
{

// Generic helper: draw a pair of edge triangles at a normalized vertical position inside a rect.
// - rc : target rectangle (triangles positioned on its left and right edges)
// - tNormalized : 0..1 vertical position inside rc
// - triWidth/triHeight : triangle size in pixels (caller should scale by ctx->scale if desired)
// - exteriorOffset : how many pixels the triangle base extends outside the rect
// - fillColor : color used for triangle fill
// - outlineColor : color used for triangle outline (drawn with cmdDrawPolyLine)
// - outlineWidth : line width for outline in pixels
static void drawEdgeTrianglesAt(
	const Rect& rc,
	f32 tNormalized,
	f32 triWidth,
	f32 triHeight,
	f32 exteriorOffset,
	const Color& fillColor,
	const Color& outlineColor = Color::black,
	f32 outlineWidth = 1.0f,
	bool drawLeft = true,
	bool drawRight = true)
{
	// compute vertical center and clamp inside rect
	f32 centerY = rc.y + tNormalized * rc.height;
	f32 halfH = triHeight * 0.5f;

	if (centerY < rc.y) centerY = rc.y;
	if (centerY > rc.bottom()) centerY = rc.bottom() ;

	// inset the tip so it sits slightly inside the rect
	f32 insetInside = triWidth * 0.75f;

	// set fill color and enqueue fill triangles
	ctx->renderer->cmdSetColor(fillColor);
	Rgba32 fillCol = ctx->renderer->currentColor;

	if (drawLeft)
	{
		// left triangle points right (tip inside rect)
		Point leftTip = { rc.x + insetInside, centerY };
		Point leftBaseTop = { rc.x - exteriorOffset, centerY - halfH };
		Point leftBaseBottom = { rc.x - exteriorOffset, centerY + halfH };

		// draw filled triangle
		ctx->renderer->cmdDrawSolidTriangle(leftBaseTop, leftBaseBottom, leftTip, fillCol, fillCol, fillCol);

		// draw outline
		Point outlinePts[3] = { leftBaseTop, leftBaseBottom, leftTip };
		LineStyle oldLs = ctx->renderer->currentLineStyle;
		LineStyle ls(outlineColor, outlineWidth);
		ctx->renderer->cmdSetLineStyle(ls);
		ctx->renderer->cmdDrawPolyLine(outlinePts, 3, true);
		// restore previous line style
		ctx->renderer->cmdSetLineStyle(oldLs);
	}

	if (drawRight)
	{
		// right triangle points left (tip inside rect)
		Point rightTip = { rc.right() - insetInside, centerY };
		Point rightBaseTop = { rc.right() + exteriorOffset, centerY - halfH };
		Point rightBaseBottom = { rc.right() + exteriorOffset, centerY + halfH };

		// draw filled triangle
		ctx->renderer->cmdSetColor(fillColor);
		fillCol = ctx->renderer->currentColor;
		ctx->renderer->cmdDrawSolidTriangle(rightTip, rightBaseTop, rightBaseBottom, fillCol, fillCol, fillCol);

		// draw outline
		Point outlinePts[3] = { rightTip, rightBaseTop, rightBaseBottom };
		LineStyle oldLs = ctx->renderer->currentLineStyle;
		LineStyle ls(outlineColor, outlineWidth);
		ctx->renderer->cmdSetLineStyle(ls);
		ctx->renderer->cmdDrawPolyLine(outlinePts, 3, true);
		// restore previous line style
		ctx->renderer->cmdSetLineStyle(oldLs);
	}
}

inline float CanonicalHue(float h)
{
	h = std::fmod(h, 1.0f);
	if (h < 0.0f) h += 1.0f;
	return h;
}

bool colorPicker(Color* inOutColor, ColorPickerFlags flags, const Color* oldColor)
{
	Color crtColor = *inOutColor;
	Color hsv = rgbToHsv(crtColor);
	f32 height = ctx->layout.width;
	ctx->id = genIdFromPosition("__COLOR_PICKER__");
	addWidget(height);
	buttonBehavior();

	if (ctx->colorPickerState.currentEditingId == ctx->id)
	{
		hsv = ctx->colorPickerState.currentHsv;
	}

	auto rcSV = ctx->widget.rect;

	rcSV.width *= 0.5f;
	rcSV.height = rcSV.width; // make it square

	auto rcH = rcSV;
	rcH.x = rcSV.right() + 10.0f * ctx->scale;
	rcH.width = 32.0f * ctx->scale;

	auto rcAlpha = rcH;

	rcAlpha.x = rcH.right() + 10.0f * ctx->scale;
	rcAlpha.width = 32.0f * ctx->scale;

	if (ctx->event.type == InputEvent::Type::MouseDown)
	{
		if (rcSV.contains(ctx->mousePosition))
		{
			ctx->colorPickerState.currentEditingId = ctx->id;
			ctx->colorPickerState.draggingElementId = 0;
			ctx->widget.captureId = ctx->id;
		}
		else if (rcH.contains(ctx->mousePosition))
		{
			ctx->colorPickerState.currentEditingId = ctx->id;
			ctx->colorPickerState.draggingElementId = 1;
			ctx->widget.captureId = ctx->id;
		}
		else if (rcAlpha.contains(ctx->mousePosition))
		{
			ctx->colorPickerState.currentEditingId = ctx->id;
			ctx->colorPickerState.draggingElementId = 2;
			ctx->widget.captureId = ctx->id;
		}
	}

	if (ctx->widget.pressed)
	{
		if (ctx->colorPickerState.draggingElementId == 0)
		{
			hsv.g = (ctx->mousePosition.x - rcSV.x) / (rcSV.width > 0.0f ? rcSV.width : 1.0f);
			hsv.b = 1.0f - (ctx->mousePosition.y - rcSV.y) / (rcSV.height > 0.0f ? rcSV.height : 1.0f);

			clampValue(hsv.g, 0.0f, 1.0f);
			clampValue(hsv.b, 0.0f, 1.0f);
		}
		else if (ctx->colorPickerState.draggingElementId == 1)
		{
			// compute normalized t and clamp immediately to avoid out-of-range hue
			hsv.r = (ctx->mousePosition.y - rcH.y) / (rcH.height > 0.0f ? rcH.height : 1.0f);
			clampValue(hsv.r, 0.0f, 1.0f);
		}
		else if (ctx->colorPickerState.draggingElementId == 2)
		{
			hsv.a = 1.0f - (ctx->mousePosition.y - rcAlpha.y) / (rcAlpha.height > 0.0f ? rcAlpha.height : 1.0f);
			clampValue(hsv.a, 0.0f, 1.0f);
		}
	}

	Color hueOnlyColor = hueToRgb(hsv.r, 1);

	ctx->renderer->cmdDrawRectangle4Colors(
		rcSV
		, Color::white, hueOnlyColor
		, hueOnlyColor, Color::white);
	ctx->renderer->cmdDrawRectangle4Colors(
		rcSV
		, Color::transparent, Color::transparent
		, Color::black, Color::black);

	Color hueColors[6] = {
		Color::red,
		Color::yellow,
		Color::green,
		Color::cyan,
		Color::blue,
		Color::magenta
	};

	for (i32 i = 0; i < 6; i++)
	{
		f32 segmentHeight = rcH.height / 6.0f;
		ctx->renderer->cmdDrawRectangle4Colors(
			{
				rcH.x,
				rcH.y + i * segmentHeight,
				rcH.width,
				segmentHeight
			},
			hueColors[i], hueColors[i],
			hueColors[(i + 1) % 6], hueColors[(i + 1) % 6]);
	}

	auto& colorPickerCheckersElem = ctx->theme->getElement(WidgetElementId::ColorPickerCheckers);
	auto& colorPickerCheckersImg = colorPickerCheckersElem.normalState().image;
	Color colorForAlphaBar = hsvToRgb({ hsv.r, hsv.g, hsv.b, 1 });
	ctx->renderer->cmdSetColor(Color::white);
	ctx->renderer->cmdDrawImageTiled(
		colorPickerCheckersImg,
		rcAlpha, Point(), ctx->scale);
	ctx->renderer->cmdDrawRectangle4Colors(rcAlpha,
		colorForAlphaBar,
		colorForAlphaBar,
		Color::transparent,
		Color::transparent);

	// Draw white triangles for hue (rcH) and alpha (rcAlpha).
	// Triangles are slightly larger; parameters tuned and scaled by ctx->scale.
	{
		f32 triW = 8.0f * ctx->scale;
		f32 triH = 15.0f * ctx->scale;
		f32 exterior = 4.0f * ctx->scale;
		f32 outlineW = 1.0f * ctx->scale;

		// hue indicators: use displayHue (preserved while dragging)
		drawEdgeTrianglesAt(rcH, hsv.r, triW, triH, exterior, Color::white, Color::black, outlineW, true, true);

		// alpha indicator: use current color alpha normalized (0..1)
		f32 alphaNorm = 1.0f - (f32)crtColor.a;
		drawEdgeTrianglesAt(rcAlpha, alphaNorm, triW, triH, exterior, Color::white, Color::black, outlineW, true, true);
	}

	auto rcSample = ctx->widget.rect;

	rcSample.x = rcAlpha.right() + 10.0f * ctx->scale;
	rcSample.width = 64;
	rcSample.height = 32;

	auto rcSampleNoAlpha = rcSample;
	auto rcSampleWithAlpha = rcSample;

	rcSampleNoAlpha.width = rcSample.width / 2.0f;
	rcSampleWithAlpha.width = rcSample.width / 2.0f;
	rcSampleWithAlpha.x = rcSampleNoAlpha.right();

	ctx->renderer->cmdSetColor(Color::white);
	ctx->renderer->cmdDrawImageTiled(
		colorPickerCheckersImg,
		rcSample, Point(), ctx->scale);

	ctx->renderer->cmdSetColor(Color{ colorForAlphaBar.r, colorForAlphaBar.g, colorForAlphaBar.b, 1 });
	ctx->renderer->cmdDrawFilledRectangle(rcSampleNoAlpha);

	ctx->renderer->cmdSetColor(Color{ colorForAlphaBar.r, colorForAlphaBar.g, colorForAlphaBar.b, hsv.a });
	ctx->renderer->cmdDrawFilledRectangle(rcSampleWithAlpha);

	Rect rcCurrentSVIndicator = rcSV;
	const f32 indicatorSize = 30.0f * ctx->scale;
	rcCurrentSVIndicator.x = rcSV.x + (hsv.g * rcSV.width - indicatorSize / 2.0f * ctx->scale);
	rcCurrentSVIndicator.y = rcSV.y + ((1.0f - hsv.b) * rcSV.height - indicatorSize / 2.0f * ctx->scale);

	rcCurrentSVIndicator.width = indicatorSize / 2.0f;
	rcCurrentSVIndicator.height = indicatorSize / 2.0f;
	ctx->renderer->cmdSetColor(hsvToRgb({ hsv.r, hsv.g, hsv.b, 1 }));
	ctx->renderer->cmdDrawFilledRectangle(rcCurrentSVIndicator);
	ctx->renderer->cmdSetLineStyle(LineStyle(Color::black, 3));
	ctx->renderer->cmdDrawRectangle(rcCurrentSVIndicator);
	ctx->renderer->cmdSetLineStyle(LineStyle(Color::white, 1));
	ctx->renderer->cmdDrawRectangle(rcCurrentSVIndicator);

	*inOutColor = hsvToRgb(hsv);

	if (ctx->colorPickerState.currentEditingId == ctx->id)
	{
		ctx->colorPickerState.currentHsv = hsv;
	}

	return true;
}

}