#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "ui_theme.h"
#include "unicode_text_cache.h"
#include "ui_font.h"
#include "ui_context.h"
#include "util.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>

namespace hui
{
bool rotarySliderFloat(const char* labelText, f32& value, f32 minVal, f32 maxVal, f32 step, bool twoSide, f32 fineStepDivideFactor)
{
	static Point lastMousePos;
	static u32 rotarySliderWidgetId = 0;
	static bool isFine = false;
	auto bodyElem = ctx->theme->getElement(WidgetElementId::RotarySliderBody);
	auto markElem = ctx->theme->getElement(WidgetElementId::RotarySliderMark);
	auto valueDotElem = ctx->theme->getElement(WidgetElementId::RotarySliderValueDot);
	bool wasModified = false;

	addWidgetItem(bodyElem.normalState().height * ctx->globalScale);
	buttonBehavior();

	if (isHovered() && ctx->event.type == InputEvent::Type::MouseDown)
	{
		lastMousePos = ctx->event.mouse.point;
		rotarySliderWidgetId = ctx->currentWidgetId;
	}

	if (ctx->event.type == InputEvent::Type::MouseUp && rotarySliderWidgetId == ctx->currentWidgetId)
	{
		rotarySliderWidgetId = 0;
		ctx->widget.changeEnded = true;
		ctx->widget.changeEnded = true;
	}

	if (ctx->event.type == InputEvent::Type::MouseMove
		&& rotarySliderWidgetId == ctx->currentWidgetId)
	{
		f32 deltaValue = 0;
		Point delta = ctx->event.mouse.point - lastMousePos;
		
		lastMousePos = ctx->event.mouse.point;

		switch (ctx->settings.sliderDragDirection)
		{
		case SliderDragDirection::Any:
			if (fabsf(delta.x) > fabsf(delta.y))
				deltaValue = delta.x;
			else
				deltaValue = ctx->settings.sliderInvertVerticalDragAmount ? delta.y : -delta.y;
			break;
		case SliderDragDirection::VerticalOnly:
			deltaValue = delta.y;
			break;
		case SliderDragDirection::HorizontalOnly:
			deltaValue = delta.x;
			break;
		}

		value += deltaValue * step * ((bool)(ctx->event.mouse.modifiers & KeyModifiers::Control) ? 1.0f / fineStepDivideFactor : 1.0f);
		wasModified = clampValue(value, minVal, maxVal);
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
		ctx->renderer->cmdSetColor(bodyElemState->color * ctx->getTint(TintColorType::Body));
		Rect rc = {
			ctx->widget.rect.x + (ctx->widget.rect.width - bodyElemState->image->width * ctx->globalScale) / 2.0f,
				ctx->widget.rect.y,
				(f32)bodyElemState->image->width * ctx->globalScale,
				(f32)bodyElemState->image->height * ctx->globalScale };
		
		ctx->renderer->cmdDrawImage(bodyElemState->image, rc);

		Point center = rc.center();
		f32 percent = 1.0f - (maxVal - value) / (maxVal - minVal);
		
		f32 limitOffset = valueDotElem.currentStyle->getParameterValue("limitOffset", 0.3f);
		f32 dotCount = valueDotElem.currentStyle->getParameterValue("count", 20);
		f32 dotPlacementRadius = valueDotElem.currentStyle->getParameterValue("placementRadius", 35);
		f32 markPlacementRadius = markElem.currentStyle->getParameterValue("placementRadius", 25);
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
		int activeDots = dotCount * percent;
		Point pos;

		if (twoSide)
		{
			Color negativeColor = valueDotElem.currentStyle->getParameterValue("negativeColor");
			Color positiveColor = valueDotElem.currentStyle->getParameterValue("positiveColor");

			angle = 1.5f * M_PI;
			step = (highLimitRadians - lowLimitRadians) / dotCount;
			activeDots = fabs(dotCount * (percent - 0.5f));
			ctx->renderer->cmdSetColor(value < 0 ? negativeColor : positiveColor);

			for (int i = 0; i <= activeDots; i++)
			{
				pos.x = cosf(angle) * dotPlacementRadius * ctx->globalScale + center.x - valueDotElem.normalState().image->width * ctx->globalScale / 2;
				pos.y = sinf(angle) * dotPlacementRadius * ctx->globalScale + center.y - valueDotElem.normalState().image->height * ctx->globalScale / 2;
				ctx->renderer->cmdDrawImage(valueDotElem.normalState().image, pos, ctx->globalScale);
				angle += step * sgn(value);
			}

			pos.x = cosf(radians) * dotPlacementRadius * ctx->globalScale + center.x - valueDotElem.normalState().image->width * ctx->globalScale / 2;
			pos.y = sinf(radians) * dotPlacementRadius * ctx->globalScale + center.y - valueDotElem.normalState().image->height * ctx->globalScale / 2;
			ctx->renderer->cmdDrawImage(valueDotElem.normalState().image, pos, ctx->globalScale);
		}
		else
		{
			for (int i = 0; i <= activeDots; i++)
			{
				pos.x = cosf(angle) * dotPlacementRadius * ctx->globalScale + center.x - valueDotElem.normalState().image->width * ctx->globalScale / 2;
				pos.y = sinf(angle) * dotPlacementRadius * ctx->globalScale + center.y - valueDotElem.normalState().image->height * ctx->globalScale / 2;
				ctx->renderer->cmdSetColor(valueDotElem.getState(WidgetStateType::Pressed).color);
				ctx->renderer->cmdDrawImage(valueDotElem.normalState().image, pos, ctx->globalScale);
				angle += step;
			}

			pos.x = cosf(radians) * dotPlacementRadius * ctx->globalScale + center.x - valueDotElem.normalState().image->width * ctx->globalScale / 2;
			pos.y = sinf(radians) * dotPlacementRadius * ctx->globalScale + center.y - valueDotElem.normalState().image->height * ctx->globalScale / 2;
			ctx->renderer->cmdSetColor(valueDotElem.getState(WidgetStateType::Pressed).color);
			ctx->renderer->cmdDrawImage(valueDotElem.normalState().image, pos, ctx->globalScale);
		}

		// draw the knob cursor
		pos.x = center.x + markPlacementRadius * cosf(radians) * ctx->globalScale - markElemState->image->width * ctx->globalScale / 2;
		pos.y = center.y + markPlacementRadius * sinf(radians) * ctx->globalScale - markElemState->image->height * ctx->globalScale / 2;

		ctx->renderer->cmdSetColor(markElemState->color);
		ctx->renderer->cmdDrawImage(markElemState->image, pos, ctx->globalScale);

		// draw the text under the knob
		ctx->renderer->cmdSetColor(bodyElemState->textColor * ctx->tint[(int)TintColorType::Text]);
		ctx->renderer->cmdSetFont(bodyElemState->font);
		ctx->renderer->pushClipRect(ctx->widget.rect);
		ctx->renderer->cmdDrawTextInBox(
			labelText,
			Rect(
				ctx->widget.rect.x,
				ctx->widget.rect.top(),
				ctx->widget.rect.width,
				ctx->widget.rect.height),
			HAlignType::Center,
			VAlignType::Bottom);
		ctx->renderer->popClipRect();
	}

	setAsFocusable();
	ctx->currentWidgetId++;

	return wasModified;
}

}