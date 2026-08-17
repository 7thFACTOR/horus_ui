#include "context.h"
#include "theme.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool check(const char* label, bool* checkVar)
{
	auto& checkBodyElem = ctx->theme->getElement(WidgetElementId::CheckBody);
	auto& checkMarkElem = ctx->theme->getElement(WidgetElementId::CheckMark);

	ctx->setLabelAndId(label);

	auto textSize = checkBodyElem.normalState().font->computeTextSize(ctx->widgetLabel.c_str());
	auto& padding = widgetGetPadding();
	f32 bulletTextSpacingParam = checkBodyElem.currentStyle->getParameter("bulletTextSpacing", ctx->settings.defaultBulletTextSpacing);
	f32 bulletTextSpacing = bulletTextSpacingParam * ctx->scale;
	f32 markWidth = padding.x * 2.0f + checkBodyElem.normalState().width;
	f32 markHeight = padding.y * 2.0f + checkBodyElem.normalState().height;
	f32 markWidthScaled = markWidth * ctx->scale;
	f32 markHeightScaled = markHeight * ctx->scale;

	// height is the same as bullet width, since its square, so we use height
	// customWidth and the height passed to addWidget() are scaled internally, so avoid double scaling here
	ctx->widget.customWidth = (markWidthScaled + textSize.width + bulletTextSpacing) / ctx->scale;
	ctx->widget.hasCustomWidth = true;

	addWidget(std::max(textSize.height, markHeightScaled) / ctx->scale);

	// set this width to just click on the bullet+text area
	ctx->widget.rect.width = ctx->widget.customWidth * ctx->scale;

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

	if (ctx->widget.disabled)
	{
		checkBodyElemState = &checkBodyElem.getState(WidgetStateType::Disabled);
		checkMarkElemState = &checkMarkElem.getState(WidgetStateType::Disabled);
	}
	else if (checkVar && *checkVar)
	{
		checkBodyElemState = &checkBodyElem.getState(WidgetStateType::Pressed);
		checkMarkElemState = &checkMarkElem.getState(WidgetStateType::Pressed);
	}
	else if (ctx->widget.hovered)
	{
		checkBodyElemState = &checkBodyElem.getState(WidgetStateType::Hovered);
		checkMarkElemState = &checkMarkElem.getState(WidgetStateType::Hovered);
	}

	ctx->renderer.cmdSetColor(checkBodyElemState->color);

	Image* bodyImage = checkBodyElemState->image;
	Image* markImage = checkMarkElemState->image;

	if (ctx->widget.disabled)
	{
		if (!bodyImage) bodyImage = checkBodyElem.normalState().image;
		if (!markImage) markImage = checkMarkElem.normalState().image;
	}

	// keep the box vertically centered on the label
	f32 boxY = ctx->widget.rect.y + (ctx->widget.rect.height - markHeightScaled) * 0.5f;

	ctx->renderer.cmdDrawImageBordered(
		bodyImage, checkBodyElemState->border,
		{
			ctx->widget.rect.x,
			boxY,
			markWidthScaled,
			markHeightScaled
		}, ctx->scale);

	if (checkVar && *checkVar)
	{
		ctx->renderer.cmdSetColor(checkMarkElemState->color);
		ctx->renderer.cmdDrawImageBordered(
			markImage,
			checkMarkElemState->border,
			{
				ctx->widget.rect.x + (markWidth - checkMarkElemState->image->width) / 2.0f * ctx->scale,
				boxY + (markHeight - checkMarkElemState->image->height) / 2.0f * ctx->scale,
				checkMarkElemState->image->width * ctx->scale,
				checkMarkElemState->image->height * ctx->scale
			}, ctx->scale);
	}

	ctx->renderer.cmdSetColor(checkBodyElemState->textColor);
	ctx->renderer.cmdSetFont(checkBodyElemState->font);
	ctx->renderer.cmdDrawTextInBox(
		ctx->widgetLabel.c_str(),
		Rect(
			ctx->widget.rect.x + markWidthScaled + bulletTextSpacing,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height),
		HAlignType::Left,
		VAlignType::Center, true);

	return ctx->widget.changeEnded;
}

}