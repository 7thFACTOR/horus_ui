#include "horus.h"
#include "types.h"
#include "theme.h"
#include "context.h"
#include "util.h"
#include <math.h>

namespace hui
{
bool sliderInternal(f32 minVal, f32 maxVal, f32& value, bool useStep, f32 step, bool isFloatValue)
{
	static bool draggingKnob = false;
	static Point dragDelta;
	auto bodyElem = ctx->theme->getElement(WidgetElementId::SliderBody);
	auto bodyFilledElem = ctx->theme->getElement(WidgetElementId::SliderBodyFilled);
	auto knobElem = ctx->theme->getElement(WidgetElementId::SliderKnob);
	bool wasModified = false;

	// clamp
	value = fmaxf(minVal, fminf(maxVal, value));

	addWidgetItem(bodyElem.normalState().height * ctx->globalScale);
	buttonBehavior();

	ctx->widget.rect.x += knobElem.normalState().image->rect.width / 2.f * ctx->globalScale;
	ctx->widget.rect.width -= knobElem.normalState().image->rect.width * ctx->globalScale;

	if (isHovered())
	{
		setMouseCursor(MouseCursorType::Hand);
	}

	f32 percentFilled = 1.0f - (maxVal - value) / (maxVal - minVal);

	Rect knobRect;
	f32 valueWidth = ctx->widget.rect.width;

	auto bodyElemState = &bodyElem.normalState();
	auto bodyFilledElemState = &bodyFilledElem.normalState();
	auto knobElemState = &knobElem.normalState();

	if (ctx->widget.focused)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Focused);
		bodyFilledElemState = &bodyFilledElem.getState(WidgetStateType::Focused);
		knobElemState = &knobElem.getState(WidgetStateType::Focused);
	}
	else if (ctx->widget.hovered)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Hovered);
		bodyFilledElemState = &bodyFilledElem.getState(WidgetStateType::Hovered);
		knobElemState = &knobElem.getState(WidgetStateType::Hovered);
	}

	knobRect = {
		ctx->widget.rect.x + valueWidth * percentFilled - knobElemState->image->rect.width / 2.0f * ctx->globalScale,
		ctx->widget.rect.y + (bodyElemState->height - knobElemState->image->rect.height) / 2.0f * ctx->globalScale,
		knobElemState->image->rect.width * ctx->globalScale,
		knobElemState->image->rect.height * ctx->globalScale
	};

	bool recomputeKnobRect = false;

	if (ctx->event.type == InputEvent::Type::MouseDown
		&& !draggingKnob
		&& ctx->isActiveLayer()
		&& ctx->hoveringThisWindow)
	{
		if (knobRect.contains(ctx->event.mouse.point))
		{
			setCapture();
			draggingKnob = true;
			dragDelta.x = ctx->event.mouse.point.x - (knobRect.x + knobRect.width / 2.0f);
		}
		else if (ctx->widget.rect.contains(ctx->event.mouse.point))
		{
			draggingKnob = true;
			dragDelta.x = 0;
			f32 t = (ctx->event.mouse.point.x - ctx->widget.rect.x) / valueWidth;
			value = minVal + (maxVal - minVal) * t;
			percentFilled = 1.0f - (maxVal - value) / (maxVal - minVal);
			recomputeKnobRect = true;
			wasModified = true;
		}
	}

	if (draggingKnob
		&& ctx->currentWidgetId == ctx->widget.focusedWidgetId
		&& ctx->isActiveLayer())
	{
		f32 x = ctx->event.mouse.point.x - dragDelta.x;

		if (x < ctx->widget.rect.x)
			x = ctx->widget.rect.x;

		if (x > ctx->widget.rect.right())
			x = ctx->widget.rect.right();

		f32 t = (x - ctx->widget.rect.x) / valueWidth;
		value = minVal + (maxVal - minVal) * t;
		percentFilled = 1.0f - (maxVal - value) / (maxVal - minVal);
		recomputeKnobRect = true;
		wasModified = true;
	}

	if (ctx->event.type == InputEvent::Type::MouseUp
		&& draggingKnob
		&& ctx->isActiveLayer())
	{
		draggingKnob = false;
		releaseCapture();
		ctx->widget.changeEnded = true;
	}

	if (recomputeKnobRect)
	{
		knobRect = {
			ctx->widget.rect.x + valueWidth * percentFilled - knobElemState->image->rect.width / 2.0f * ctx->globalScale,
			ctx->widget.rect.y + (bodyElemState->height - knobElemState->image->rect.height) / 2.0f * ctx->globalScale,
			knobElemState->image->rect.width * ctx->globalScale,
			knobElemState->image->rect.height * ctx->globalScale
		};
	}

	ctx->renderer->cmdSetColor(bodyElemState->color);
	ctx->renderer->cmdDrawImageBordered(
		bodyElemState->image,
		bodyElemState->border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y + (bodyElemState->height - bodyElemState->image->rect.height) / 2.0f * ctx->globalScale,
			ctx->widget.rect.width,
			bodyElemState->image->rect.height * ctx->globalScale
		},
		ctx->globalScale);

	ctx->renderer->cmdSetColor(bodyFilledElemState->color);
	ctx->renderer->cmdDrawImageBordered(
		bodyFilledElemState->image,
		bodyFilledElemState->border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y + (bodyElemState->height - bodyFilledElemState->image->rect.height) / 2.f * ctx->globalScale,
			ctx->widget.rect.width * percentFilled,
			bodyFilledElemState->image->rect.height * ctx->globalScale
		},
		ctx->globalScale);

	ctx->renderer->cmdSetColor(knobElemState->color);
	ctx->renderer->cmdDrawImageBordered(
		knobElemState->image,
		knobElemState->border,
		knobRect,
		ctx->globalScale);
	setFocusable();
	ctx->currentWidgetId++;

	return wasModified;
}

bool sliderInteger(i32 minVal, i32 maxVal, i32& value, bool useStep, i32 step)
{
	f32 val = value;
	bool ret = sliderInternal(minVal, maxVal, val, useStep, step, false);
	value = val;
	return ret;
}

bool sliderFloat(f32 minVal, f32 maxVal, f32& value, bool useStep, f32 step)
{
	return sliderInternal(minVal, maxVal, value, useStep, step, true);
}

}
