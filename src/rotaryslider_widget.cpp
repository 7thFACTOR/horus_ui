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
bool rotarySliderFloat(const char* labelText, f32& value, f32 minVal, f32 maxVal, f32 step)
{
	static Point lastMousePos;
	static u32 rotarySliderWidgetId = 0;
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
	}

	if (ctx->event.type == InputEvent::Type::MouseMove && rotarySliderWidgetId == ctx->currentWidgetId)
	{
		f32 deltaValue = 0;
		Point delta = ctx->event.mouse.point - lastMousePos;
		lastMousePos = ctx->event.mouse.point;

		if (fabsf(delta.x) > fabsf(delta.y))
			deltaValue = delta.x;
		else
			deltaValue = ctx->settings.sliderInvertVerticalDragAmount ? delta.y : -delta.y;

		value += deltaValue * step;
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
		ctx->renderer->cmdSetColor(bodyElemState->color * ctx->tint[(int)TintColorType::Body]);
		Rect rc = {
			ctx->widget.rect.x + (ctx->widget.rect.width - bodyElemState->image->width) / 2.0f,
				ctx->widget.rect.y,
				(f32)bodyElemState->image->width,
				(f32)bodyElemState->image->height };
		ctx->renderer->cmdDrawImage(bodyElemState->image, rc);

		Point center = rc.center();
		f32 percent = 1.0f - (maxVal - value) / (maxVal - minVal);
		
		Point qpos[4] = {
			Point(0, 0),
			Point(markElemState->image->width, 0),
			Point(markElemState->image->width, markElemState->image->height),
			Point(0, markElemState->image->height),
		};

		f32 lowLimitRadians = M_PI / 2 + 0.3f;
		f32 highLimitRadians = 2 * M_PI + M_PI / 2 - 0.3f;
		f32 radians = lowLimitRadians + percent * (highLimitRadians - lowLimitRadians);
		int numDots = 22;
		f32 step = (highLimitRadians - lowLimitRadians) / (f32)numDots;
		f32 angle = lowLimitRadians;
		int activeDots = numDots * percent;

		for (int i = 0; i <= numDots; i++)
		{
			Point pos;
			
			pos.x = cosf(angle) * 34 + center.x - valueDotElem.normalState().image->width / 2;
			pos.y = sinf(angle) * 34 + center.y - valueDotElem.normalState().image->height / 2;
			if (i <= activeDots)
			{
				ctx->renderer->cmdSetColor(valueDotElem.getState(WidgetStateType::Pressed).color);
			}
			else
			{
				ctx->renderer->cmdSetColor(valueDotElem.getState(WidgetStateType::Normal).color);
			}

			ctx->renderer->cmdDrawImage(valueDotElem.normalState().image, pos);

			angle += step;
		}

		for (int i = 0; i < 4; i++)
		{
			Point newp;
			qpos[i].y -= markElemState->image->height / 2;
			qpos[i].x += markElem.normalState().height;
			newp.x = cosf(radians) * qpos[i].x - sinf(radians) * qpos[i].y;
			newp.y = sinf(radians) * qpos[i].x + cosf(radians) * qpos[i].y;
			qpos[i] = newp + center;
		}

		ctx->renderer->cmdSetColor(markElemState->color);
		ctx->renderer->cmdDrawQuad(markElemState->image, qpos[0], qpos[1], qpos[2], qpos[3]);
		
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