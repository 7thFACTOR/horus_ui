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

	ctx->setLabelAndId(label);

	auto textSize = checkBodyElem.normalState().font->computeTextSize(ctx->widgetLabel.c_str());
	auto& padding = getWidgetPadding();
	f32 bulletTextSpacingParam = checkBodyElem.currentStyle->getParameter("bulletTextSpacing", ctx->settings.defaultBulletTextSpacing);
	f32 bulletTextSpacing = bulletTextSpacingParam * ctx->scale;
	f32 markWidth = padding.x * 2.0f + checkBodyElem.normalState().width;
	f32 markHeight = padding.y * 2.0f + checkBodyElem.normalState().height;
	f32 markWidthScaled = markWidth * ctx->scale;
	f32 markHeightScaled = markHeight * ctx->scale;

	// height is the same as bullet width, since its square, so we use height
	ctx->widget.customWidth = markWidthScaled + textSize.width + bulletTextSpacing;
	ctx->widget.hasCustomWidth = true;

	addWidget(std::max(textSize.height, markHeightScaled));

	// set this width to just click on the bullet+text area
	ctx->widget.rect.width = ctx->widget.customWidth;

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

	ctx->renderer.cmdSetColor(checkBodyElemState->color);
	ctx->renderer.cmdDrawImageBordered(
		checkBodyElemState->image, checkBodyElemState->border,
		{
			round(ctx->widget.rect.x),
			round(ctx->widget.rect.y),
			markWidthScaled,
			markHeightScaled
		}, ctx->scale);

	if (checkVar && *checkVar)
	{
		ctx->renderer.cmdSetColor(checkMarkElemState->color);
		ctx->renderer.cmdDrawImageBordered(
			checkMarkElemState->image,
			checkMarkElemState->border,
			{
				ctx->widget.rect.x + (markWidth - checkMarkElemState->image->width) / 2.0f * ctx->scale,
				ctx->widget.rect.y + (markHeight - checkMarkElemState->image->height) / 2.0f * ctx->scale,
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