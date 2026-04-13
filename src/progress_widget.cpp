#include "context.h"
#include "theme.h"
#include "util.h"
#include "font.h"

namespace hui
{
void progress(f32 value, f32 maxValue, bool showText, bool showRealValues, const char* indeterminateText)
{
	auto& backElem = ctx->theme->getElement(WidgetElementId::ProgressBack);
	auto& fillElem = ctx->theme->getElement(WidgetElementId::ProgressFill);
	auto& padding = widgetGetPadding();

	if (ctx->sameLine.enabled && !ctx->widget.hasNextWidth)
	{
		ctx->widget.customWidth = ((backElem.normalState().border + padding.x) * 2.0f) * ctx->scale;
		ctx->widget.hasCustomWidth = true;
	}

	ctx->setLabelAndId(nullptr);
	addWidget((backElem.normalState().height + padding.y * 2.0) * ctx->scale);
	buttonBehavior();

	const bool isSubUnit = value <= 1.0f && maxValue == 0.0f;
	const bool isIndeterminate = value < 0;

	if (isSubUnit)
	{
		value = std::min(value, 1.0f);
	}
	else
	{
		value = std::min(value, maxValue);
	}

	f32 percentFilled = isSubUnit ? value : value / maxValue;
	f32 percentValueWidth = percentFilled * ctx->widget.rect.width;
	f32 animPosX = 0;

	if (isIndeterminate)
	{
		percentValueWidth = ctx->widget.rect.width / 6;
		animPosX = fmodf(ctx->totalTime * fabsf(value) * 1500.0f + ctx->widget.rect.width + percentValueWidth, ctx->widget.rect.width + percentValueWidth) - percentValueWidth;
	}

	auto& backElemState = backElem.normalState();
	auto& fillElemState = fillElem.normalState();

	ctx->renderer.cmdSetColor(tintApply(backElemState.color, TintColorType::Body));
	ctx->renderer.cmdDrawImageBordered(backElemState.image, backElemState.border,
		ctx->widget.rect, ctx->scale);

	Rect fillRc = {
			ctx->widget.rect.x + animPosX,
			ctx->widget.rect.y + (ctx->widget.rect.height - (fillElemState.height * ctx->scale + padding.y * 2.0f * ctx->scale)) / 2.0f,
			percentValueWidth,
			fillElemState.height * ctx->scale + padding.y * 2.0f * ctx->scale };

	if (isIndeterminate)
	{
		if (fillRc.x < ctx->widget.rect.x)
		{
			auto startX = ctx->widget.rect.x;
			fillRc.width = fillRc.right() - startX;
			fillRc.x = startX;
		}

		if (fillRc.right() > ctx->widget.rect.right())
		{
			fillRc.width -= fillRc.right() - ctx->widget.rect.right();
		}
	}

	ctx->renderer.cmdSetColor(tintApply(fillElemState.color, TintColorType::Body));
	ctx->renderer.cmdDrawImageBordered(fillElemState.image, fillElemState.border,
		fillRc, ctx->scale);

	std::string text;

	if (isIndeterminate)
	{
		text = indeterminateText ? indeterminateText : "";
	}
	else
	if (isSubUnit || !showRealValues)
	{
		text = std::to_string((u32)(percentFilled * 100)) + "%";
	}
	else
	if (!isSubUnit && showRealValues)
	{
		text = std::to_string((u32)value) + "/" + std::to_string((u32)maxValue);
	}

	Color textShadowColor = fillElem.currentStyle->getColorParameter("textShadowColor", Color::black);

	if (!isIndeterminate)
	{
		FontTextSize fsize = fillElemState.font->computeTextSize(text.c_str());
		Rect textRc = fillRc;
		f32 spacing = fillElem.currentStyle->getParameter("barTextSpacing", 4.0f);

		textRc.width += fsize.width + spacing * ctx->scale;
		const f32 rightSide = ctx->widget.rect.right() - backElemState.border * ctx->scale;

		if (textRc.right() >= rightSide)
		{
			textRc.width = rightSide - textRc.x;
		}

		ctx->renderer.cmdSetFont(fillElemState.font);
		ctx->renderer.cmdSetColor(tintApply(textShadowColor, TintColorType::Text));
		ctx->renderer.cmdDrawTextInBox(text.c_str(),
			textRc, HAlignType::Right, VAlignType::Center);

		ctx->renderer.cmdSetColor(tintApply(fillElemState.textColor, TintColorType::Text));
		textRc -= Point(1, 1);
		ctx->renderer.cmdDrawTextInBox(text.c_str(),
			textRc, HAlignType::Right, VAlignType::Center);
	}
	else
	{
		Rect textRc = ctx->widget.rect;

		ctx->renderer.cmdSetColor(tintApply(textShadowColor, TintColorType::Text));
		ctx->renderer.cmdDrawTextInBox(text.c_str(),
			ctx->widget.rect, HAlignType::Center, VAlignType::Center);

		ctx->renderer.cmdSetColor(tintApply(fillElemState.textColor, TintColorType::Text));
		textRc -= Point(1, 1);
		ctx->renderer.cmdDrawTextInBox(text.c_str(),
			textRc, HAlignType::Center, VAlignType::Center);
	}
}

}
