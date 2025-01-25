#include "context.h"
#include "theme.h"
#include "util.h"
#include <math.h>

namespace hui
{
bool sliderInternal(const char* id, f32 minVal, f32 maxVal, f32& value, bool useStep, f32 step, bool isFloatValue)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::SliderBody);
	auto& bodyFilledElem = ctx->theme->getElement(WidgetElementId::SliderBodyFilled);
	auto& knobElem = ctx->theme->getElement(WidgetElementId::SliderKnob);
	bool wasModified = false;

	// clamp
	value = fmaxf(minVal, fminf(maxVal, value));

	ctx->id = genId(id);
	addWidget(bodyElem.normalState().height * ctx->scale);
	buttonBehavior();

	ctx->widget.rect.x += knobElem.normalState().image->rect.width / 2.f * ctx->scale;
	ctx->widget.rect.width -= knobElem.normalState().image->rect.width * ctx->scale;

	if (isHovered())
	{
		setMouseCursor(MouseCursorType::HandPointing);
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
		ctx->widget.rect.x + valueWidth * percentFilled - knobElemState->image->rect.width / 2.0f * ctx->scale,
		ctx->widget.rect.y + (bodyElemState->height - knobElemState->image->rect.height) / 2.0f * ctx->scale,
		knobElemState->image->rect.width * ctx->scale,
		knobElemState->image->rect.height * ctx->scale
	};

	bool recomputeKnobRect = false;

	if (ctx->event.type == InputEvent::Type::MouseDown
		&& !ctx->slider.draggingKnob
		&& ctx->isActiveLayer()
		&& ctx->hoveringThisWindow)
	{
		if (knobRect.contains(ctx->mousePosition))
		{
			setCapture();
			ctx->slider.draggingKnob = true;
			ctx->slider.dragDelta.x = ctx->mousePosition.x - (knobRect.x + knobRect.width / 2.0f);
		}
		else if (ctx->widget.rect.contains(ctx->mousePosition))
		{
			ctx->slider.draggingKnob = true;
			ctx->slider.dragDelta.x = 0;
			f32 t = (ctx->mousePosition.x - ctx->widget.rect.x) / valueWidth;
			value = minVal + (maxVal - minVal) * t;
			percentFilled = 1.0f - (maxVal - value) / (maxVal - minVal);
			recomputeKnobRect = true;
			wasModified = true;
		}
	}

	if (ctx->slider.draggingKnob
		&& ctx->id == ctx->widget.focusedId
		&& ctx->isActiveLayer())
	{
		f32 x = ctx->mousePosition.x - ctx->slider.dragDelta.x;

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
		&& ctx->slider.draggingKnob
		&& ctx->isActiveLayer())
	{
		ctx->slider.draggingKnob = false;
		releaseCapture();
		ctx->widget.changeEnded = true;
	}

	if (recomputeKnobRect)
	{
		knobRect = {
			ctx->widget.rect.x + valueWidth * percentFilled - knobElemState->image->rect.width / 2.0f * ctx->scale,
			ctx->widget.rect.y + (bodyElemState->height - knobElemState->image->rect.height) / 2.0f * ctx->scale,
			knobElemState->image->rect.width * ctx->scale,
			knobElemState->image->rect.height * ctx->scale
		};
	}

	ctx->renderer->cmdSetColor(bodyElemState->color);
	ctx->renderer->cmdDrawImageBordered(
		bodyElemState->image,
		bodyElemState->border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y + (bodyElemState->height - bodyElemState->image->rect.height) / 2.0f * ctx->scale,
			ctx->widget.rect.width,
			bodyElemState->image->rect.height * ctx->scale
		},
		ctx->scale);

	ctx->renderer->cmdSetColor(bodyFilledElemState->color);
	ctx->renderer->cmdDrawImageBordered(
		bodyFilledElemState->image,
		bodyFilledElemState->border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y + (bodyElemState->height - bodyFilledElemState->image->rect.height) / 2.f * ctx->scale,
			ctx->widget.rect.width * percentFilled,
			bodyFilledElemState->image->rect.height * ctx->scale
		},
		ctx->scale);

	ctx->renderer->cmdSetColor(knobElemState->color);
	ctx->renderer->cmdDrawImageBordered(
		knobElemState->image,
		knobElemState->border,
		knobRect,
		ctx->scale);
	setFocusable();

	return wasModified;
}

bool sliderInteger(const char* id, i32 minVal, i32 maxVal, i32& value, bool useStep, i32 step)
{
	f32 val = value;
	bool ret = sliderInternal(id, minVal, maxVal, val, useStep, step, false);
	value = val;

	return ret;
}

bool sliderFloat(const char* id, f32 minVal, f32 maxVal, f32& value, bool useStep, f32 step)
{
	return sliderInternal(id, minVal, maxVal, value, useStep, step, true);
}

}
