#include "context.h"
#include "theme.h"
#include "util.h"
#include "font.h"

namespace hui
{
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
	ctx->renderer.cmdSetColor(fillColor);
	Rgba32 fillCol = ctx->renderer.currentColor;

	if (drawLeft)
	{
		// left triangle points right (tip inside rect)
		Point leftTip = { rc.x + insetInside, centerY };
		Point leftBaseTop = { rc.x - exteriorOffset, centerY - halfH };
		Point leftBaseBottom = { rc.x - exteriorOffset, centerY + halfH };

		// draw filled triangle
		ctx->renderer.cmdDrawSolidTriangle(leftBaseTop, leftBaseBottom, leftTip, fillCol, fillCol, fillCol);

		// draw outline
		Point outlinePts[3] = { leftBaseTop, leftBaseBottom, leftTip };
		LineStyle oldLs = ctx->renderer.currentLineStyle;
		LineStyle ls(outlineColor, outlineWidth);
		ctx->renderer.cmdSetLineStyle(ls);
		ctx->renderer.cmdDrawPolyLine(outlinePts, 3, true);
		// restore previous line style
		ctx->renderer.cmdSetLineStyle(oldLs);
	}

	if (drawRight)
	{
		// right triangle points left (tip inside rect)
		Point rightTip = { rc.right() - insetInside, centerY };
		Point rightBaseTop = { rc.right() + exteriorOffset, centerY - halfH };
		Point rightBaseBottom = { rc.right() + exteriorOffset, centerY + halfH };

		// draw filled triangle
		ctx->renderer.cmdSetColor(fillColor);
		fillCol = ctx->renderer.currentColor;
		ctx->renderer.cmdDrawSolidTriangle(rightTip, rightBaseTop, rightBaseBottom, fillCol, fillCol, fillCol);

		// draw outline
		Point outlinePts[3] = { rightTip, rightBaseTop, rightBaseBottom };
		LineStyle oldLs = ctx->renderer.currentLineStyle;
		LineStyle ls(outlineColor, outlineWidth);
		ctx->renderer.cmdSetLineStyle(ls);
		ctx->renderer.cmdDrawPolyLine(outlinePts, 3, true);
		// restore previous line style
		ctx->renderer.cmdSetLineStyle(oldLs);
	}
}

static void drawColorPreviewSwatch(const Rect& rc, const Color& color, const char* text)
{
	auto& colorPickerState = ctx->theme->getElement(WidgetElementId::ColorPickerBody).normalState();
	auto& colorPickerCheckersState = ctx->theme->getElement(WidgetElementId::ColorPickerCheckers).normalState();
	auto tsize = colorPickerState.font->computeTextSize(text);

	ctx->renderer.cmdSetColor(colorPickerState.textColor);
	ctx->renderer.cmdSetFont(colorPickerState.font);
	ctx->renderer.cmdDrawTextInBox(
		text,
		{
			rc.x,
			rc.y,
			rc.width,
			tsize.height * ctx->scale
		},
		HAlignType::Left,
		VAlignType::Center);

	Rect rcSample = {
		rc.x,
		rc.y + tsize.height * ctx->scale + 5.0f * ctx->scale,
		64 * ctx->scale,
		32.0f * ctx->scale
	};

	auto rcSampleNoAlpha = rcSample;
	auto rcSampleWithAlpha = rcSample;

	rcSampleNoAlpha.width = rcSample.width / 2.0f;
	rcSampleWithAlpha.width = rcSample.width / 2.0f;
	rcSampleWithAlpha.x = rcSampleNoAlpha.right();

	ctx->renderer.cmdSetColor(Color::white);
	ctx->renderer.cmdDrawImageTiled(
		colorPickerCheckersState.image,
		rcSample, Point(), ctx->scale);

	ctx->renderer.cmdSetColor(Color{ color.r, color.g, color.b, 1 });
	ctx->renderer.cmdDrawFilledRectangle(rcSampleNoAlpha);

	ctx->renderer.cmdSetColor(color);
	ctx->renderer.cmdDrawFilledRectangle(rcSampleWithAlpha);
}

bool colorPicker(const char* id, Color* inOutColor, ColorPickerFlags flags, const Color* oldColor)
{
	//TODO: move constants to settings or theme
	const f32 indicatorSize = 20.0f * ctx->scale;
	Color crtColor = *inOutColor;
	i32 crtIntR = (u32)(crtColor.r * 255.0f);
	i32 crtIntG = (u32)(crtColor.g * 255.0f);
	i32 crtIntB = (u32)(crtColor.b * 255.0f);
	i32 crtIntA = (u32)(crtColor.a * 255.0f);
	Color hsv = colorRgbToHsv(crtColor);
	f32 height = ctx->layout.width * 0.5f + indicatorSize;
	ctx->id = genId(id);
	
	auto pickerId = ctx->id;
	
	addWidget(height);
	buttonBehavior();

	if (ctx->colorPickerState.currentEditingId == pickerId)
	{
		hsv = ctx->colorPickerState.currentHsv;
		crtColor = ctx->colorPickerState.currentRgb;
		crtIntR = ctx->colorPickerState.intR;
		crtIntG = ctx->colorPickerState.intG;
		crtIntB = ctx->colorPickerState.intB;
		crtIntA = ctx->colorPickerState.intA;
	}

	std::string hexColorStr;

	hexColorStr = colorToHex(crtColor);
	std::snprintf(ctx->colorPickerState.hexColor, ColorPickerState::maxHexColorSize, hexColorStr.c_str());

	auto rcSV = ctx->widget.rect;
	auto clippedRc = ctx->widget.rect.clipInside(ctx->renderer.getClipRect());
	rcSV.x += indicatorSize / 2.0f + 1.0f;
	rcSV.y += indicatorSize / 2.0f;
	rcSV.width *= 0.5f;
	rcSV.height = rcSV.width; // make it square

	auto rcH = rcSV;
	rcH.x = rcSV.right() + 10.0f * ctx->scale;
	rcH.width = 32.0f * ctx->scale;

	auto rcAlpha = rcH;

	rcAlpha.x = rcH.right() + 10.0f * ctx->scale;
	rcAlpha.width = 32.0f * ctx->scale;

	auto clippedRcSV = rcSV.clipInside(ctx->renderer.getClipRect());
	auto clippedRcHue = rcH.clipInside(ctx->renderer.getClipRect());
	auto clippedRcAlpha = rcAlpha.clipInside(ctx->renderer.getClipRect());

	if (ctx->event.type == InputEvent::Type::MouseDown)
	{
		if (clippedRcSV.contains(ctx->mousePosition))
		{
			ctx->colorPickerState.currentEditingId = ctx->id;
			ctx->colorPickerState.draggingElementId = 0;
			ctx->widget.captureId = ctx->id;
		}
		else if (clippedRcHue.contains(ctx->mousePosition))
		{
			ctx->colorPickerState.currentEditingId = ctx->id;
			ctx->colorPickerState.draggingElementId = 1;
			ctx->widget.captureId = ctx->id;
		}
		else if (clippedRcAlpha.contains(ctx->mousePosition))
		{
			ctx->colorPickerState.currentEditingId = ctx->id;
			ctx->colorPickerState.draggingElementId = 2;
			ctx->widget.captureId = ctx->id;
		}
		else if(clippedRc.contains(ctx->mousePosition))
		{
			ctx->colorPickerState.draggingElementId = ~0;
			ctx->widget.captureId = 0;
		}
	}

	bool hueChanged = false;
	bool svChanged = false;
	bool alphaChanged = false;

	if (ctx->widget.pressed)
	{
		if (ctx->colorPickerState.draggingElementId == 0)
		{
			hsv.g = (ctx->mousePosition.x - rcSV.x) / (rcSV.width > 0.0f ? rcSV.width : 1.0f);
			hsv.b = 1.0f - (ctx->mousePosition.y - rcSV.y) / (rcSV.height > 0.0f ? rcSV.height : 1.0f);

			clampValue(hsv.g, 0.0f, 1.0f);
			clampValue(hsv.b, 0.0f, 1.0f);
			svChanged = true;
		}
		else if (ctx->colorPickerState.draggingElementId == 1)
		{
			// compute normalized t and clamp immediately to avoid out-of-range hue
			hsv.r = (ctx->mousePosition.y - rcH.y) / (rcH.height > 0.0f ? rcH.height : 1.0f);
			clampValue(hsv.r, 0.0f, 1.0f);
			hueChanged = true;
		}
		else if (ctx->colorPickerState.draggingElementId == 2)
		{
			hsv.a = 1.0f - (ctx->mousePosition.y - rcAlpha.y) / (rcAlpha.height > 0.0f ? rcAlpha.height : 1.0f);
			clampValue(hsv.a, 0.0f, 1.0f);
			alphaChanged = true;
		}

		if (hueChanged || svChanged || alphaChanged)
		{
			crtColor = colorHsvToRgb(hsv);

			if (!has(flags, ColorPickerFlags::Float))
			{
				ctx->colorPickerState.intR = crtIntR = crtColor.r * 255;
				ctx->colorPickerState.intG = crtIntG = crtColor.g * 255;
				ctx->colorPickerState.intB = crtIntB = crtColor.b * 255;
				ctx->colorPickerState.intA = crtIntA = crtColor.a * 255;
			}

			std::string hexColorStr = colorToHex(crtColor);
			std::snprintf(ctx->colorPickerState.hexColor, ColorPickerState::maxHexColorSize, hexColorStr.c_str());
		}
	}

	auto clampedHsv = hsv;

	clampedHsv.r = clampValue01(clampedHsv.r);
	clampedHsv.g = clampValue01(clampedHsv.g);
	clampedHsv.b = clampValue01(clampedHsv.b);

	Color hueOnlyColor = colorHueToRgb(clampedHsv.r, 1);

	ctx->renderer.cmdDrawRectangle4Colors(
		rcSV
		, Color::white, hueOnlyColor
		, hueOnlyColor, Color::white);
	ctx->renderer.cmdDrawRectangle4Colors(
		rcSV
		, Color::transparent, Color::transparent
		, Color::black, Color::black);

	static const Color hueColors[6] = {
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
		ctx->renderer.cmdDrawRectangle4Colors(
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
	Color colorForAlphaBar = colorHsvToRgb({ clampedHsv.r, clampedHsv.g, clampedHsv.b, 1 });

	ctx->renderer.cmdSetColor(Color::white);
	ctx->renderer.cmdDrawImageTiled(
		colorPickerCheckersImg,
		rcAlpha, Point(), ctx->scale);
	ctx->renderer.cmdDrawRectangle4Colors(rcAlpha,
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
		drawEdgeTrianglesAt(rcH, clampedHsv.r, triW, triH, exterior, Color::white, Color::black, outlineW, true, true);

		// alpha indicator: use current color alpha normalized (0..1)
		f32 alphaNorm = 1.0f - (f32)clampedHsv.a;
		drawEdgeTrianglesAt(rcAlpha, alphaNorm, triW, triH, exterior, Color::white, Color::black, outlineW, true, true);
	}

	drawColorPreviewSwatch(
		{
			rcAlpha.right() + 10.0f * ctx->scale,
			rcAlpha.y,
			64,
			64
		},
		colorHsvToRgb(clampedHsv),
		"Preview");

	if (oldColor)
	{
		drawColorPreviewSwatch(
			{
				rcAlpha.right() + 10.0f * ctx->scale,
				rcAlpha.y + 64 * ctx->scale,
				64,
				64
			},
			*oldColor,
			"Old");
	}

	Rect rcCurrentSVIndicator = rcSV;

	rcCurrentSVIndicator.x = rcSV.x + clampedHsv.g * rcSV.width - indicatorSize / 2.0f * ctx->scale;
	rcCurrentSVIndicator.y = rcSV.y + (1.0f - clampedHsv.b) * rcSV.height - indicatorSize / 2.0f * ctx->scale;
	rcCurrentSVIndicator.width = indicatorSize;
	rcCurrentSVIndicator.height = indicatorSize;
	ctx->renderer.cmdSetColor(colorHsvToRgb({ clampedHsv.r, clampedHsv.g, clampedHsv.b, 1 }));
	ctx->renderer.cmdDrawFilledRectangle(rcCurrentSVIndicator);
	ctx->renderer.cmdSetLineStyle(LineStyle(Color::black, 3));
	ctx->renderer.cmdDrawRectangle(rcCurrentSVIndicator);
	ctx->renderer.cmdSetLineStyle(LineStyle(Color::white, 1));
	ctx->renderer.cmdDrawRectangle(rcCurrentSVIndicator);

	bool hsvChanged = false;

	hui::sameLineGroupBegin(3);

	if (hui::comboSliderFloat(&hsv.r, 0.001f, 0.001f, "H: %.4f"))
	{
		hsvChanged = true;
	}

	hui::sameLineGroupNext();

	if (hui::comboSliderFloat(&hsv.g, 0.001f, 0.001f, "S: %.4f"))
	{
		hsvChanged = true;
	}

	hui::sameLineGroupNext();

	if (hui::comboSliderFloat(&hsv.b, 0.001f, 0.001f, "V: %.4f"))
	{
		hsvChanged = true;
	}

	hui::sameLineGroupEnd();

	if (hsvChanged)
	{
		ctx->colorPickerState.currentEditingId = pickerId;

		if (!(flags & ColorPickerFlags::Hdr))
		{
			hsv.r = clampValue01(hsv.r);
		}
		
		if (!(flags & ColorPickerFlags::Hdr))
		{
			hsv.g = clampValue01(hsv.g);
		}

		if (!(flags & ColorPickerFlags::Hdr))
		{
			hsv.b = clampValue01(hsv.b);
		}

		crtColor = colorHsvToRgb(hsv);

		std::string hexColorStr;

		if (!has(flags, ColorPickerFlags::Float))
		{
			ctx->colorPickerState.intR = crtIntR = crtColor.r * 255;
			ctx->colorPickerState.intG = crtIntG = crtColor.g * 255;
			ctx->colorPickerState.intB = crtIntB = crtColor.b * 255;
			ctx->colorPickerState.intA = crtIntA = crtColor.a * 255;
		}

		hexColorStr = colorToHex(crtColor);
		std::snprintf(ctx->colorPickerState.hexColor, ColorPickerState::maxHexColorSize, hexColorStr.c_str());
	}

	bool rgbaChanged = false;

	if (has(flags, ColorPickerFlags::Float))
	{
		if (has(flags, ColorPickerFlags::Hdr))
		{
			hui::sameLineGroupBegin(4);

			if (hui::comboSliderFloat(&crtColor.r, 0.001f, 0.001f, "R: %.4f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupNext();

			if (hui::comboSliderFloat(&crtColor.g, 0.001f, 0.001f, "G: %.4f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupNext();

			if (hui::comboSliderFloat(&crtColor.b, 0.001f, 0.001f, "B: %.4f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupNext();

			if (hui::comboSliderFloat(&crtColor.a, 0.001f, 0.001f, "A: %.4f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupEnd();
		}
		else
		{
			hui::sameLineGroupBegin(4);

			if (hui::comboSliderFloatRanged(&crtColor.r, 0, 1, 0.001f, 0.001f, "R: %.4f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupNext();

			if (hui::comboSliderFloatRanged(&crtColor.g, 0, 1, 0.001f, 0.001f, "G: %.4f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupNext();

			if (hui::comboSliderFloatRanged(&crtColor.b, 0, 1, 0.001f, 0.001f, "B: %.4f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupNext();

			if (hui::comboSliderFloatRanged(&crtColor.a, 0, 1, 0.001f, 0.001f, "A: %.4f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupEnd();
		}
	}
	else
	{
		if (has(flags, ColorPickerFlags::Hdr))
		{
			hui::sameLineGroupBegin(4);

			if (hui::comboSliderInt(&crtIntR, 0.1f, 1, "R: %.0f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupNext();

			if (hui::comboSliderInt(&crtIntG, 0.1f, 1, "G: %.0f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupNext();

			if (hui::comboSliderInt(&crtIntB, 0.1f, 1, "B: %.0f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupNext();

			if (hui::comboSliderInt(&crtIntA, 0.1f, 1, "A: %.0f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupEnd();
		}
		else
		{
			hui::sameLineGroupBegin(4);

			if (hui::comboSliderIntRanged(&crtIntR, 0, 255, 0.1f, 1, "R: %.0f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupNext();

			if (hui::comboSliderIntRanged(&crtIntG, 0, 255, 0.1f, 1, "G: %.0f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupNext();

			if (hui::comboSliderIntRanged(&crtIntB, 0, 255, 0.1f, 1, "B: %.0f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupNext();

			if (hui::comboSliderIntRanged(&crtIntA, 0, 255, 0.1f, 1, "A: %.0f"))
			{
				rgbaChanged = true;
			}

			hui::sameLineGroupEnd();
		}
	}

	if (rgbaChanged)
	{
		if (!has(flags, ColorPickerFlags::Float))
		{
			ctx->colorPickerState.intR = crtIntR;
			ctx->colorPickerState.intG = crtIntG;
			ctx->colorPickerState.intB = crtIntB;
			ctx->colorPickerState.intA = crtIntA;
			
			crtColor = Color::fromU8(
				ctx->colorPickerState.intR,
				ctx->colorPickerState.intG,
				ctx->colorPickerState.intB,
				ctx->colorPickerState.intA);
		}

		ctx->colorPickerState.currentHsv = hsv = colorRgbToHsv(crtColor);
		ctx->colorPickerState.currentEditingId = pickerId;
		std::string hexColorStr;

		hexColorStr = colorToHex(crtColor);
		std::snprintf(ctx->colorPickerState.hexColor, ColorPickerState::maxHexColorSize, hexColorStr.c_str());
	}

	if (hui::textInput("colorPicker_hexColorEdit", ctx->colorPickerState.hexColor, ColorPickerState::maxHexColorSize, TextInputFlags::HexOnly))
	{
		crtColor = colorFromHex(ctx->colorPickerState.hexColor);
		ctx->colorPickerState.currentHsv = hsv = colorRgbToHsv(crtColor);

		if (!has(flags, ColorPickerFlags::Float))
		{
			ctx->colorPickerState.intR = crtIntR = crtColor.r * 255;
			ctx->colorPickerState.intG = crtIntG = crtColor.g * 255;
			ctx->colorPickerState.intB = crtIntB = crtColor.b * 255;
			ctx->colorPickerState.intA = crtIntA = crtColor.a * 255;
		}

		ctx->colorPickerState.currentRgb = crtColor;
		ctx->colorPickerState.currentEditingId = pickerId;
	}

	if (ctx->colorPickerState.currentEditingId == pickerId)
	{
		ctx->colorPickerState.currentHsv = hsv;
		ctx->colorPickerState.currentRgb = crtColor;
		ctx->colorPickerState.intR = crtIntR;
		ctx->colorPickerState.intG = crtIntG;
		ctx->colorPickerState.intB = crtIntB;
		ctx->colorPickerState.intA = crtIntA;
	}

	*inOutColor = crtColor;

	return true;
}

}