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
bool sliderRotary(const char* label, f32* value, f32 minVal, f32 maxVal, f32 step, bool twoSide, f32 fineStepDivideFactor)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::RotarySliderBody);
	auto& markElem = ctx->theme->getElement(WidgetElementId::RotarySliderMark);
	auto& valueDotElem = ctx->theme->getElement(WidgetElementId::RotarySliderValueDot);
	bool wasModified = false;
	auto& padding = widgetPaddingGet();

	if (ctx->sameLine.enabled && !ctx->widget.hasNextWidth)
	{
		ctx->widget.customWidth = ((bodyElem.normalState().border + padding.x) * 2.0f) * ctx->scale;
		ctx->widget.hasCustomWidth = true;
	}

	ctx->labelAndIdSet(label);
	widgetAdd((bodyElem.normalState().height + padding.y * 2.0f) * ctx->scale);
	buttonBehavior();

	if (widgetIsHovered() && ctx->event.type == InputEvent::Type::MouseDown)
	{
		ctx->rotarySlider.lastMousePos = ctx->mousePosition;
		ctx->rotarySlider.id = ctx->id;
	}

	if (ctx->event.type == InputEvent::Type::MouseUp && ctx->rotarySlider.id == ctx->id)
	{
		ctx->rotarySlider.id = 0;
		ctx->widget.changeEnded = true;
	}

	if (ctx->event.type == InputEvent::Type::MouseMove
		&& ctx->rotarySlider.id == ctx->id)
	{
		f32 deltaValue = 0;
		Point delta = ctx->mousePosition - ctx->rotarySlider.lastMousePos;
		
		ctx->rotarySlider.lastMousePos = ctx->mousePosition;

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
	}
	
	auto bodyElemState = &bodyElem.normalState();
	auto markElemState = &markElem.normalState();

	if (ctx->widget.pressed)
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

		if (twoSide)
		{
			Color negativeColor = valueDotElem.currentStyle->getColorParameter("negativeColor");
			Color positiveColor = valueDotElem.currentStyle->getColorParameter("positiveColor");

			angle = 1.5f * M_PI;
			step = (highLimitRadians - lowLimitRadians) / dotCount;
			activeDots = fabs(dotCount * (percent - 0.5f));
			ctx->renderer.cmdSetColor(*value < 0 ? negativeColor : positiveColor);

			for (i32 i = 0; i <= activeDots; i++)
			{
				pos.x = cosf(angle) * dotPlacementRadius * ctx->scale + center.x - valueDotElem.normalState().image->width * ctx->scale / 2;
				pos.y = sinf(angle) * dotPlacementRadius * ctx->scale + center.y - valueDotElem.normalState().image->height * ctx->scale / 2;
				ctx->renderer.cmdDrawImage(valueDotElem.normalState().image, pos, ctx->scale);
				angle += step * (value ? sgn(*value) : 1.0f);
			}

			pos.x = cosf(radians) * dotPlacementRadius * ctx->scale + center.x - valueDotElem.normalState().image->width * ctx->scale / 2;
			pos.y = sinf(radians) * dotPlacementRadius * ctx->scale + center.y - valueDotElem.normalState().image->height * ctx->scale / 2;
			ctx->renderer.cmdDrawImage(valueDotElem.normalState().image, pos, ctx->scale);
		}
		else
		{
			for (i32 i = 0; i <= activeDots; i++)
			{
				pos.x = cosf(angle) * dotPlacementRadius * ctx->scale + center.x - valueDotElem.normalState().image->width * ctx->scale / 2;
				pos.y = sinf(angle) * dotPlacementRadius * ctx->scale + center.y - valueDotElem.normalState().image->height * ctx->scale / 2;
				ctx->renderer.cmdSetColor(valueDotElem.getState(WidgetStateType::Pressed).color);
				ctx->renderer.cmdDrawImage(valueDotElem.normalState().image, pos, ctx->scale);
				angle += step;
			}

			pos.x = cosf(radians) * dotPlacementRadius * ctx->scale + center.x - valueDotElem.normalState().image->width * ctx->scale / 2;
			pos.y = sinf(radians) * dotPlacementRadius * ctx->scale + center.y - valueDotElem.normalState().image->height * ctx->scale / 2;
			ctx->renderer.cmdSetColor(valueDotElem.getState(WidgetStateType::Pressed).color);
			ctx->renderer.cmdDrawImage(valueDotElem.normalState().image, pos, ctx->scale);
		}

		// draw the knob cursor
		pos.x = center.x + markPlacementRadius * cosf(radians) * ctx->scale - markElemState->image->width * ctx->scale / 2;
		pos.y = center.y + markPlacementRadius * sinf(radians) * ctx->scale - markElemState->image->height * ctx->scale / 2;

		ctx->renderer.cmdSetColor(markElemState->color);
		ctx->renderer.cmdDrawImage(markElemState->image, pos, ctx->scale);

		// draw the text under the knob
		ctx->renderer.cmdSetColor(tintApply(bodyElemState->textColor, TintColorType::Text));
		ctx->renderer.cmdSetFont(bodyElemState->font);
		ctx->renderer.pushClipRect(ctx->widget.rect);
		ctx->renderer.cmdDrawTextInBox(
			ctx->widgetLabel.c_str(),
			Rect(
				ctx->widget.rect.x,
				ctx->widget.rect.top(),
				ctx->widget.rect.width,
				ctx->widget.rect.height),
			HAlignType::Center,
			VAlignType::Bottom);
		ctx->renderer.popClipRect();
	}

	focusableSet();

	return wasModified;
}

}
