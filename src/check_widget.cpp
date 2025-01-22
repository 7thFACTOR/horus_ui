#include "context.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool check(const char* label, bool* checkVar)
{
	auto& checkBodyElem = ctx->theme->getElement(WidgetElementId::CheckBody);
	auto& checkMarkElem = ctx->theme->getElement(WidgetElementId::CheckMark);

	addWidgetItem(label, checkBodyElem.normalState().height * ctx->scale);
	buttonBehavior();
	ctx->widget.changeEnded = false;

	if (ctx->widget.clicked)
	{
		if (checkVar)
			*checkVar = !*checkVar;
		
		forceRepaint();
		ctx->widget.changeEnded = true;
	}

	auto checkBodyElemState = &checkBodyElem.normalState();
	auto checkMarkElemState = &checkMarkElem.normalState();

	if (checkVar && *checkVar)
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
		}, ctx->scale);

	if (checkVar && *checkVar)
	{
		ctx->renderer->cmdSetColor(checkMarkElemState->color);
		ctx->renderer->cmdDrawImageBordered(
			checkMarkElemState->image,
			checkMarkElemState->border,
			{
				ctx->widget.rect.x + (checkBodyElemState->width - checkMarkElemState->image->rect.width) / 2.0f * ctx->scale,
				ctx->widget.rect.y + (checkBodyElemState->height - checkMarkElemState->image->rect.height) / 2.0f * ctx->scale,
				checkMarkElemState->image->rect.width * ctx->scale,
				checkMarkElemState->image->rect.height * ctx->scale
			}, ctx->scale);
	}

	const f32 bulletTextSpacingParam = checkBodyElem.currentStyle->getParameterValue("bulletTextSpacing", 5);
	const f32 bulletTextSpacing = bulletTextSpacingParam * ctx->scale;

	ctx->renderer->cmdSetColor(checkBodyElemState->textColor);
	ctx->renderer->cmdSetFont(checkBodyElemState->font);
	ctx->renderer->cmdDrawTextInBox(
		ctx->widgetLabel.c_str(),
		Rect(
			ctx->widget.rect.x + ctx->widget.rect.height + bulletTextSpacing,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height),
		HAlignType::Left,
		VAlignType::Center);

	return ctx->widget.changeEnded;
}

}