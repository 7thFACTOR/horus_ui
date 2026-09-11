#include "context.h"
#include "theme.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool check(const char* label, bool* checkVar, bool* indeterminate)
{
	auto& checkBodyElem = ctx->theme->getElement(WidgetElementId::CheckBody);
	auto& checkMarkElem = ctx->theme->getElement(WidgetElementId::CheckMark);
	auto& checkMarkIndeterminateElem = ctx->theme->getElement(WidgetElementId::CheckMarkIndeterminate);

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

	bool isChecked = checkVar != nullptr && *checkVar;
	bool isIndeterminate = indeterminate != nullptr && *indeterminate;

	if (ctx->widget.clicked)
	{
		// clicking an indeterminate check marks it as checked, clicking a
		// checked or unchecked one clears the indeterminate state
		if (isIndeterminate)
		{
			if (checkVar)
				*checkVar = true;
			if (indeterminate)
				*indeterminate = false;
		}
		else
		{
			if (checkVar)
				*checkVar = !*checkVar;
			if (indeterminate)
				*indeterminate = false;
		}

		forceRepaint();
		ctx->widget.changeEnded = true;
	}

	auto checkBodyElemState = &checkBodyElem.normalState();
	auto checkMarkElemState = &checkMarkElem.normalState();
	auto checkMarkIndeterminateElemState = &checkMarkIndeterminateElem.normalState();

	if (ctx->widget.disabled)
	{
		checkBodyElemState = &checkBodyElem.getState(WidgetStateType::Disabled);
		checkMarkElemState = &checkMarkElem.getState(WidgetStateType::Disabled);
		checkMarkIndeterminateElemState = &checkMarkIndeterminateElem.getState(WidgetStateType::Disabled);
	}
	else if (isChecked || isIndeterminate)
	{
		checkBodyElemState = &checkBodyElem.getState(WidgetStateType::Pressed);
		checkMarkElemState = &checkMarkElem.getState(WidgetStateType::Pressed);
		checkMarkIndeterminateElemState = &checkMarkIndeterminateElem.getState(WidgetStateType::Pressed);
	}
	else if (ctx->widget.hovered)
	{
		checkBodyElemState = &checkBodyElem.getState(WidgetStateType::Hovered);
		checkMarkElemState = &checkMarkElem.getState(WidgetStateType::Hovered);
		checkMarkIndeterminateElemState = &checkMarkIndeterminateElem.getState(WidgetStateType::Hovered);
	}

	ctx->renderer.cmdSetColor(checkBodyElemState->color);

	Image* bodyImage = checkBodyElemState->image;

	if (ctx->widget.disabled && !bodyImage)
		bodyImage = checkBodyElem.normalState().image;

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

	if (isChecked || isIndeterminate)
	{
		auto* markElemState = isIndeterminate ? checkMarkIndeterminateElemState : checkMarkElemState;

		if (markElemState->image)
		{
			auto markImage = markElemState->image;
			ctx->renderer.cmdSetColor(markElemState->color);
			ctx->renderer.cmdDrawImageBordered(
				markImage,
				markElemState->border,
				{
					ctx->widget.rect.x + (markWidth - markImage->width) / 2.0f * ctx->scale,
					boxY + (markHeight - markImage->height) / 2.0f * ctx->scale,
					markImage->width * ctx->scale,
					markImage->height * ctx->scale
				}, ctx->scale);
		}
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