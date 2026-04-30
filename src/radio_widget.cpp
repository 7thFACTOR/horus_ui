#include "context.h"
#include "theme.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool radio(const char* label, i32* currentRadioValue, i32 thisValue)
{
	auto& radioBodyElem = ctx->theme->getElement(WidgetElementId::RadioBody);
	auto& radioMarkElem = ctx->theme->getElement(WidgetElementId::RadioMark);

	ctx->setLabelAndId(label);
	
	auto textSize = radioBodyElem.normalState().font->computeTextSize(ctx->widgetLabel.c_str());
	auto& padding = widgetGetPadding();
	f32 bulletTextSpacingParam = radioBodyElem.currentStyle->getParameter("bulletTextSpacing", ctx->settings.defaultBulletTextSpacing);
	f32 bulletTextSpacing = bulletTextSpacingParam * ctx->scale;
	f32 height = radioBodyElem.normalState().height * ctx->scale;
	f32 markWidth = padding.x * 2.0f + radioBodyElem.normalState().width;
	f32 markHeight = padding.y * 2.0f + radioBodyElem.normalState().height;
	f32 markWidthScaled = markWidth * ctx->scale;
	f32 markHeightScaled = markHeight * ctx->scale;

	// height is the same as bullet width, since its square, so we use height
	if (!ctx->widget.hasNextWidth)
	{
		ctx->widget.customWidth = markWidthScaled + textSize.width + bulletTextSpacing;
		ctx->widget.hasCustomWidth = true;
	}
	
	addWidget(std::max(textSize.height, markHeightScaled));
	buttonBehavior();
	ctx->widget.changeEnded = false;

	if (ctx->widget.clicked)
	{
		if (currentRadioValue)
			*currentRadioValue = thisValue;
		
		ctx->widget.changeEnded = true;
		forceRepaint();
	}

	auto radioBodyElemState = &radioBodyElem.normalState();
	auto radioMarkElemState = &radioMarkElem.normalState();

	if (ctx->widget.disabled)
	{
		radioBodyElemState = &radioBodyElem.getState(WidgetStateType::Disabled);
		radioMarkElemState = &radioMarkElem.getState(WidgetStateType::Disabled);
	}
	else if (currentRadioValue && *currentRadioValue == thisValue)
	{
		radioBodyElemState = &radioBodyElem.getState(WidgetStateType::Pressed);
		radioMarkElemState = &radioMarkElem.getState(WidgetStateType::Pressed);
	}
	else if (ctx->widget.hovered)
	{
		radioBodyElemState = &radioBodyElem.getState(WidgetStateType::Hovered);
		radioMarkElemState = &radioMarkElem.getState(WidgetStateType::Hovered);
	}

	ctx->renderer.cmdSetColor(radioBodyElemState->color);

	Image* bodyImage = radioBodyElemState->image;
	Image* markImage = radioMarkElemState->image;

	if (ctx->widget.disabled)
	{
		if (!bodyImage) bodyImage = radioBodyElem.normalState().image;
		if (!markImage) markImage = radioMarkElem.normalState().image;
	}

	ctx->renderer.cmdDrawImageBordered(
		bodyImage,
		radioBodyElemState->border,
		{
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			markWidthScaled,
			markHeightScaled
		}, ctx->scale);

	if (currentRadioValue && *currentRadioValue == thisValue)
	{
		ctx->renderer.cmdSetColor(radioMarkElemState->color);
		ctx->renderer.cmdDrawImageBordered(
			markImage, radioMarkElemState->border,
			{
				ctx->widget.rect.x + (markWidth - markImage->width) / 2.0f * ctx->scale,
				ctx->widget.rect.y + (markHeight - markImage->height) / 2.0f * ctx->scale,
				markImage->rect.width * ctx->scale,
				markImage->rect.height * ctx->scale
			}, ctx->scale);
	}

	ctx->renderer.cmdSetColor(radioBodyElemState->textColor);
	ctx->renderer.cmdSetFont(radioBodyElemState->font);
	ctx->renderer.cmdDrawTextInBox(
		ctx->widgetLabel.c_str(),
		Rect(
			ctx->widget.rect.x + markWidthScaled + bulletTextSpacing,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height),
		HAlignType::Left,
		VAlignType::Center);

	ctx->widget.width = 0;

	return ctx->widget.changeEnded;
}

}