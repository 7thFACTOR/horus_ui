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
	auto bodyElem = ctx->theme->getElement(WidgetElementId::RotarySliderBody);
	auto markElem = ctx->theme->getElement(WidgetElementId::RotarySliderMark);

	addWidgetItem(bodyElem.normalState().height * ctx->globalScale);
	buttonBehavior();

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

		center.x -= markElemState->image->width / 2;
		center.y -= markElemState->image->height / 2;

		Point pos;
		f32 radius = 21;

		pos.x = center.x + sinf(value*M_PI) * radius;
		pos.y = center.y + cosf(value*M_PI) * radius;

		Point qpos[4] = {
			Point(-5, 0),
			Point(5, 0),
			Point(5, 35),
			Point(-5, 35)
		};
		value += 0.01f;
		for (int i = 0; i < 4; i++)
		{
			Point newp;
			//qpos[i] -= center;
			newp.x = cosf(value*M_PI) * qpos[i].x - sinf(value*M_PI) * qpos[i].y;
			newp.y = sinf(value*M_PI) * qpos[i].x - cosf(value*M_PI) * qpos[i].y;
			qpos[i] = newp + center;
		}

		//ctx->renderer->cmdDrawImage(markElemState->image, pos);
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

	return ctx->widget.clicked;
}

}