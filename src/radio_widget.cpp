#include "horus.h"
#include "types.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "context.h"
#include "util.h"

namespace hui
{
bool radio(const char* labelText, bool checked)
{
	auto radioBodyElem = ctx->theme->getElement(WidgetElementId::RadioBody);
	auto radioMarkElem = ctx->theme->getElement(WidgetElementId::RadioMark);

	addWidgetItem(radioBodyElem.normalState().height * ctx->globalScale);
	buttonBehavior();

	if (ctx->widget.clicked)
	{
		checked = !checked;
		ctx->widget.changeEnded = true;
		forceRepaint();
	}

	auto radioBodyElemState = &radioBodyElem.normalState();
	auto radioMarkElemState = &radioMarkElem.normalState();

	if (checked)
	{
		radioBodyElemState = &radioBodyElem.getState(WidgetStateType::Pressed);
		radioMarkElemState = &radioMarkElem.getState(WidgetStateType::Pressed);
	}
	else if (ctx->widget.hovered)
	{
		radioBodyElemState = &radioBodyElem.getState(WidgetStateType::Hovered);
		radioMarkElemState = &radioMarkElem.getState(WidgetStateType::Hovered);
	}

	ctx->renderer->cmdSetColor(radioBodyElemState->color);
	ctx->renderer->cmdDrawImageBordered(
		radioBodyElemState->image,
		radioBodyElemState->border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			radioBodyElemState->width * ctx->globalScale,
			ctx->widget.rect.height
		}, ctx->globalScale);

	if (checked)
	{
		ctx->renderer->cmdSetColor(radioMarkElemState->color);
		ctx->renderer->cmdDrawImageBordered(
			radioMarkElemState->image, radioMarkElemState->border,
			{
				ctx->widget.rect.x + (radioBodyElemState->width - radioMarkElemState->image->rect.width) / 2.0f * ctx->globalScale,
				ctx->widget.rect.y + (radioBodyElemState->height - radioMarkElemState->image->rect.height) / 2.0f * ctx->globalScale,
				radioMarkElemState->image->rect.width * ctx->globalScale,
				radioMarkElemState->image->rect.height * ctx->globalScale
			}, ctx->globalScale);
	}

	const f32 bulletTextSpacingParam = radioBodyElem.currentStyle->getParameterValue("bulletTextSpacing", 5);
	const f32 bulletTextSpacing = bulletTextSpacingParam * ctx->globalScale;

	ctx->renderer->cmdSetColor(radioBodyElemState->textColor);
	ctx->renderer->cmdSetFont(radioBodyElemState->font);
	ctx->renderer->cmdDrawTextInBox(
		labelText,
		Rect(
			ctx->widget.rect.x + ctx->widget.rect.height + bulletTextSpacing,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height),
		HAlignType::Left,
		VAlignType::Center);

	ctx->currentWidgetId++;

	return checked;
}

}