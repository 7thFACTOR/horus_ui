#include "context.h"
#include "renderer.h"
#include "theme.h"
#include "font.h"
#include "util.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>

namespace hui
{
bool circularSliderFloat(const char* label, f32* value, f32 minVal, f32 maxVal, f32 step, bool twoSide, f32 fineStepDivideFactor, CircularSliderFlags flags)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::CircularSliderBody);
	auto& markElem = ctx->theme->getElement(WidgetElementId::CircularSliderMark);
	auto& valueDotElem = ctx->theme->getElement(WidgetElementId::CircularSliderValueDot);
	bool wasModified = false;
	auto& padding = widgetGetPadding();

	if (!ctx->widget.hasNextWidth)
	{
		ctx->widget.customWidth = bodyElem.normalState().image->width;
		ctx->widget.hasCustomWidth = true;
	}

	ctx->setLabelAndId(label);

	f32 labelSpacing = bodyElem.currentStyle->getParameter("labelSpacing", ctx->settings.defaultCircularSliderLabelSpacing);
	f32 labelSpace = 0;

	if (ctx->widgetLabel[0])
	{
		labelSpace = labelSpacing;

		if (bodyElem.normalState().font)
			labelSpace += bodyElem.normalState().font->getMetrics().height;
	}

	addWidget((bodyElem.normalState().height + padding.y * 2.0f + labelSpace) * ctx->scale);
	buttonBehavior();

	if (!ctx->widget.disabled && widgetIsHovered() && ctx->event.type == InputEvent::Type::MouseDown)
	{
		ctx->circularSlider.lastMousePos = ctx->mousePosition;
		ctx->circularSlider.id = ctx->id;
		ctx->circularSlider.hiddenCursorPos = ctx->settings.services.getAbsoluteMousePosition();
		ctx->settings.services.hideMouseCursor();
		windowSetCapture();
	}

	if (ctx->event.type == InputEvent::Type::MouseUp && ctx->circularSlider.id == ctx->id && !ctx->widget.disabled)
	{
		ctx->circularSlider.id = 0;
		ctx->widget.changeEnded = true;
		ctx->settings.services.setAbsoluteMousePosition(ctx->circularSlider.hiddenCursorPos);
		ctx->settings.services.showMouseCursor();
		windowReleaseCapture();
	}

	if (ctx->event.type == InputEvent::Type::MouseMove
		&& ctx->circularSlider.id == ctx->id
		&& !ctx->widget.disabled)
	{
		f32 deltaValue = 0;
		Point delta = ctx->mousePosition - ctx->circularSlider.lastMousePos;
		
		ctx->circularSlider.lastMousePos = ctx->mousePosition;

		switch (ctx->settings.sliderDragDirection)
		{
		case SliderDragDirection::Any:
			if (fabsf(delta.x) > fabsf(delta.y))
			{
				deltaValue = delta.x;
			}
			else
			{
				deltaValue = ctx->settings.sliderInvertVerticalDragAmount ? delta.y : -delta.y;
			}

			break;
		case SliderDragDirection::VerticalOnly:
			deltaValue = delta.y;
			break;
		case SliderDragDirection::HorizontalOnly:
			deltaValue = delta.x;
			break;
		}

		*value += deltaValue * step * ((bool)(ctx->event.mouse.modifiers & KeyModifiers::Control) ? 1.0f / fineStepDivideFactor : 1.0f);
		wasModified = clampValue(*value, minVal, maxVal);

		if (ctx->lastHoveredNativeWindow)
		{
			auto wndPos = ctx->settings.services.getWindowPosition(ctx->lastHoveredNativeWindow);
			auto wndSize = ctx->settings.services.getWindowSize(ctx->lastHoveredNativeWindow);
			auto absPos = ctx->settings.services.getAbsoluteMousePosition();
			const f32 margin = 2.0f;

			if (absPos.x <= wndPos.x + margin || absPos.x >= wndPos.x + wndSize.x - margin)
			{
				Point centerScreen(wndPos.x + wndSize.x * 0.5f, absPos.y);
				ctx->settings.services.setAbsoluteMousePosition(centerScreen);
				Point warpDelta = centerScreen - absPos;
				ctx->circularSlider.lastMousePos += warpDelta;
				ctx->mousePosition += warpDelta;
			}
		}
	}
	
	auto bodyElemState = &bodyElem.normalState();
	auto markElemState = &markElem.normalState();
	auto valueDotElemState = &valueDotElem.normalState();

	if (ctx->widget.disabled)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Disabled);
		markElemState = &markElem.getState(WidgetStateType::Disabled);
		valueDotElemState = &valueDotElem.getState(WidgetStateType::Disabled);
	}
	else if (ctx->widget.pressed)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);
		markElemState = &markElem.getState(WidgetStateType::Pressed);
	}
	else if (ctx->widget.focused)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Focused);
		markElemState = &markElem.getState(WidgetStateType::Focused);
	}
	else if (ctx->widget.hovered)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Hovered);
		markElemState = &markElem.getState(WidgetStateType::Hovered);
	}

	if (ctx->widget.visible)
	{
		ctx->renderer.cmdSetColor(tintApply(bodyElemState->color, TintColorType::Body));
		Rect rc = {
			ctx->widget.rect.x + (ctx->widget.rect.width - bodyElemState->image->width * ctx->scale) / 2.0f,
				ctx->widget.rect.y,
				(f32)bodyElemState->image->width * ctx->scale,
				(f32)bodyElemState->image->height * ctx->scale };
		
		ctx->renderer.cmdDrawImage(bodyElemState->image, rc);

		Point center = rc.center();
		f32 percent = 1.0f - (maxVal - *value) / (maxVal - minVal);
		
		f32 limitOffset = valueDotElem.currentStyle->getParameter("limitOffset", 0.3f);
		f32 dotCount = valueDotElem.currentStyle->getParameter("count", 20);
		f32 dotPlacementRadius = valueDotElem.currentStyle->getParameter("placementRadius", 35);
		f32 markPlacementRadius = markElem.currentStyle->getParameter("placementRadius", 25);
		f32 lowLimitRadians;
		f32 highLimitRadians;

		if (!twoSide)
		{
			lowLimitRadians = M_PI / 2 + limitOffset;
			highLimitRadians = 2 * M_PI + M_PI / 2 - limitOffset;
		}
		else
		{
			lowLimitRadians = M_PI * 0.5f;
			highLimitRadians = 2.5f * M_PI;
		}

		f32 radians = lowLimitRadians + percent * (highLimitRadians - lowLimitRadians);
		f32 step = (highLimitRadians - lowLimitRadians) / dotCount;
		f32 angle = lowLimitRadians;
		i32 activeDots = dotCount * percent;
		Point pos;
		Color dotColor;

		if (twoSide)
		{
			Color negativeColor = valueDotElem.currentStyle->getColorParameter("negativeColor");
			Color positiveColor = valueDotElem.currentStyle->getColorParameter("positiveColor");

			angle = 1.5f * M_PI;
			step = (highLimitRadians - lowLimitRadians) / dotCount;
			activeDots = fabs(dotCount * (percent - 0.5f));
			dotColor = ctx->widget.disabled ? valueDotElemState->color : (*value < 0 ? negativeColor : positiveColor);
			ctx->renderer.cmdSetColor(dotColor);

			for (i32 i = 0; i <= activeDots; i++)
			{
				pos.x = cosf(angle) * dotPlacementRadius * ctx->scale + center.x - valueDotElemState->image->width * ctx->scale / 2;
				pos.y = sinf(angle) * dotPlacementRadius * ctx->scale + center.y - valueDotElemState->image->height * ctx->scale / 2;
				ctx->renderer.cmdDrawImage(valueDotElemState->image, pos, ctx->scale);
				angle += step * (value ? sgn(*value) : 1.0f);
			}

			pos.x = cosf(radians) * dotPlacementRadius * ctx->scale + center.x - valueDotElemState->image->width * ctx->scale / 2;
			pos.y = sinf(radians) * dotPlacementRadius * ctx->scale + center.y - valueDotElemState->image->height * ctx->scale / 2;
			ctx->renderer.cmdDrawImage(valueDotElemState->image, pos, ctx->scale);
		}
		else
		{
			dotColor = ctx->widget.disabled ? valueDotElemState->color : valueDotElem.getState(WidgetStateType::Pressed).color;

			for (i32 i = 0; i <= activeDots; i++)
			{
				pos.x = cosf(angle) * dotPlacementRadius * ctx->scale + center.x - valueDotElemState->image->width * ctx->scale / 2;
				pos.y = sinf(angle) * dotPlacementRadius * ctx->scale + center.y - valueDotElemState->image->height * ctx->scale / 2;
				ctx->renderer.cmdSetColor(dotColor);
				ctx->renderer.cmdDrawImage(valueDotElemState->image, pos, ctx->scale);
				angle += step;
			}

			pos.x = cosf(radians) * dotPlacementRadius * ctx->scale + center.x - valueDotElemState->image->width * ctx->scale / 2;
			pos.y = sinf(radians) * dotPlacementRadius * ctx->scale + center.y - valueDotElemState->image->height * ctx->scale / 2;
			ctx->renderer.cmdSetColor(dotColor);
			ctx->renderer.cmdDrawImage(valueDotElemState->image, pos, ctx->scale);
		}

		// draw the knob cursor
		pos.x = center.x + markPlacementRadius * cosf(radians) * ctx->scale - markElemState->image->width * ctx->scale / 2;
		pos.y = center.y + markPlacementRadius * sinf(radians) * ctx->scale - markElemState->image->height * ctx->scale / 2;

		ctx->renderer.cmdSetColor(markElemState->color);
		ctx->renderer.cmdDrawImage(markElemState->image, pos, ctx->scale);

		// draw value in center if flagged
		if (has(flags, CircularSliderFlags::ShowValueInCenter))
		{
			char valStr[64];
			snprintf(valStr, sizeof(valStr), "%.0f%%", *value);

			Rect centerRect = {
				rc.x,
				rc.y,
				rc.width,
				rc.height
			};

			ctx->renderer.cmdSetColor(tintApply(bodyElemState->textColor, TintColorType::Text));
			ctx->renderer.cmdSetFont(bodyElemState->font);
			ctx->renderer.pushClipRect(rc);
			ctx->renderer.cmdDrawTextInBox(valStr, centerRect, HAlignType::Center, VAlignType::Center);
			ctx->renderer.popClipRect();
		}

		// draw the text under the knob
		if (ctx->widgetLabel[0])
		{
			Rect labelRect(
				ctx->widget.rect.x,
				rc.bottom() + labelSpacing * ctx->scale,
				ctx->widget.rect.width,
				ctx->widget.rect.bottom() - (rc.bottom() + labelSpacing * ctx->scale));

			ctx->renderer.cmdSetColor(tintApply(bodyElemState->textColor, TintColorType::Text));
			ctx->renderer.cmdSetFont(bodyElemState->font);
			ctx->renderer.pushClipRect(ctx->widget.rect);
			ctx->renderer.cmdDrawTextInBox(ctx->widgetLabel.c_str(), labelRect, HAlignType::Center, VAlignType::Bottom);
			ctx->renderer.popClipRect();
		}
	}

	widgetSetFocusable();

	return wasModified;
}

}
