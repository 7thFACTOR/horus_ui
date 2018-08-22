#include "horus.h"
#include "types.h"
#include "ui_theme.h"
#include "text_cache.h"
#include "ui_font.h"
#include "ui_context.h"
#include "util.h"

namespace hui
{
bool check(Utf8String labelText, bool checked)
{
	auto checkBodyElem = ctx->theme->getElement(WidgetElementId::CheckBody);
	auto checkMarkElem = ctx->theme->getElement(WidgetElementId::CheckMark);

	addWidgetItem(checkBodyElem.normalState().height * ctx->globalScale);
	buttonBehavior();

	if (ctx->widget.clicked)
		checked = !checked;

	auto checkBodyElemState = &checkBodyElem.normalState();
	auto checkMarkElemState = &checkMarkElem.normalState();

	if (checked)
	{
		checkBodyElemState = &checkBodyElem.getState(WidgetStateType::Pressed);
		checkMarkElemState = &checkMarkElem.getState(WidgetStateType::Pressed);
	}
	else if (ctx->widget.hovered)
	{
		checkBodyElemState = &checkBodyElem.getState(WidgetStateType::Hovered);
		checkMarkElemState = &checkMarkElem.getState(WidgetStateType::Hovered);
	}

	ctx->renderer->cmdSetColor(checkBodyElemState->color);
	ctx->renderer->cmdDrawImageBordered(
		checkBodyElemState->image, checkBodyElemState->border,
		{
			round(ctx->widget.rect.x),
			round(ctx->widget.rect.y),
			ctx->widget.rect.height,
			ctx->widget.rect.height
		}, ctx->globalScale);

	if (checked)
	{
		ctx->renderer->cmdSetColor(checkMarkElemState->color);
		ctx->renderer->cmdDrawImageBordered(
			checkMarkElemState->image,
			checkMarkElemState->border,
			{
				ctx->widget.rect.x + (checkBodyElemState->width - checkMarkElemState->image->rect.width) / 2.0f * ctx->globalScale,
				ctx->widget.rect.y + (checkBodyElemState->height - checkMarkElemState->image->rect.height) / 2.0f * ctx->globalScale,
				checkMarkElemState->image->rect.width * ctx->globalScale,
				checkMarkElemState->image->rect.height * ctx->globalScale
			}, ctx->globalScale);
	}

	const f32 bulletTextSpacing = ctx->settings.checkBulletTextSpacing * ctx->globalScale;

	ctx->renderer->cmdSetColor(checkBodyElemState->textColor);
	ctx->renderer->cmdSetFont(checkBodyElemState->font);
	ctx->renderer->cmdDrawTextInBox(
		labelText,
		Rect(
			ctx->widget.rect.x + (checkBodyElemState->border * 2.0f + checkBodyElemState->image->rect.width) * ctx->globalScale + bulletTextSpacing,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height),
		HAlignType::Left,
		VAlignType::Center);

	ctx->currentWidgetId++;

	return checked;
}

}