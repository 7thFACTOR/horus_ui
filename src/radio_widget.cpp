#include "context.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool radio(const char* label, i32* currentRadioValue, i32 thisValue)
{
	auto& radioBodyElem = ctx->theme->getElement(WidgetElementId::RadioBody);
	auto& radioMarkElem = ctx->theme->getElement(WidgetElementId::RadioMark);

	addWidgetItem(label, radioBodyElem.normalState().height * ctx->globalScale);
	buttonBehavior();
	bool changed = false;

	if (ctx->widget.clicked)
	{
		if (currentRadioValue)
			*currentRadioValue = thisValue;
		
		ctx->widget.changeEnded = true;
		forceRepaint();
		changed = true;
	}

	auto radioBodyElemState = &radioBodyElem.normalState();
	auto radioMarkElemState = &radioMarkElem.normalState();

	if (currentRadioValue && *currentRadioValue == thisValue)
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

	if (currentRadioValue && *currentRadioValue == thisValue)
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
		ctx->widgetLabel.c_str(),
		Rect(
			ctx->widget.rect.x + ctx->widget.rect.height + bulletTextSpacing,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height),
		HAlignType::Left,
		VAlignType::Center);

	return changed;
}

}