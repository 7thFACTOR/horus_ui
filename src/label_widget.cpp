#include "context.h"
#include "theme.h"
#include "font.h"
#include "unicode_text_cache.h"
#include "util.h"

namespace hui
{
static bool labelInternal(const char* label, HAlignType horizontalAlign, Font* font)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::LabelBody);
	f32 height = 0;
	auto& bodyElemState = bodyElem.normalState();

	ctx->setLabelAndId(label);
	auto fsize = ctx->renderer.computeSizeOrDrawText(ctx->widgetLabel.c_str(), Rect(0, 0, ctx->layout.width , FLT_MAX), HAlignType::Left, VAlignType::Top, false, font, true);
	height = (bodyElemState.height > fsize.height ? bodyElemState.height : fsize.height);

	if (!ctx->widget.hasNextWidth)
	{
		ctx->widget.customWidth = fsize.width;
		ctx->widget.hasCustomWidth = true;
	}

	addWidget(height);
	buttonBehavior();

	if (ctx->widget.hoveredId == ctx->id)
	{
		ctx->widget.hoveredType = WidgetType::Label;
	}

	if (ctx->widget.visible)
	{
		Rect textRc = {
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height
		};

		ctx->renderer.cmdSetColor(applyTint(bodyElemState.textColor, TintColorType::Text));
		ctx->renderer.cmdSetFont(font ? font : bodyElemState.font);
		ctx->renderer.cmdDrawTextInBox(
			ctx->widgetLabel.c_str(),
			textRc,
			horizontalAlign,
			VAlignType::Center, true);
	}

	return ctx->widget.clicked;
}

bool label(const char* label, HAlignType horizontalAlign)
{
	return labelInternal(label, horizontalAlign, nullptr);
}

bool labelCustomFont(const char* label, HFont font, HAlignType horizontalAlign)
{
	return labelInternal(label, horizontalAlign, (Font*)font);
}

bool labelMultiline(const char* label, HAlignType horizontalAlign)
{
	auto& bodyElemState = ctx->theme->getElement(WidgetElementId::LabelBody).normalState();

	return labelCustomFontMultiline(label, bodyElemState.font, horizontalAlign);
}

bool labelCustomFontMultiline(const char* label, HFont font, HAlignType horizontalAlign)
{
	auto& bodyElemState = ctx->theme->getElement(WidgetElementId::LabelBody).normalState();
	auto& padding = getWidgetPadding();
	f32 width = ctx->layout.width - padding.x * 2.0f * ctx->scale;

	ctx->setLabelAndId(label);

	auto textSize = ((Font*)font)->computeTextSize(ctx->widgetLabel.c_str(), (u32)round(width));

	addWidget(textSize.height + padding.y * 2.0f * ctx->scale);

	ctx->renderer.cmdSetColor(applyTint(bodyElemState.textColor, TintColorType::Text));
	ctx->renderer.cmdSetFont((Font*)font);
	ctx->renderer.cmdDrawTextInBox(
		ctx->widgetLabel.c_str(),
		{
			ctx->widget.rect.x + padding.x * ctx->scale,
			ctx->widget.rect.y + padding.y * ctx->scale,
			width,
			0,
		},
		horizontalAlign,
		VAlignType::Top);

	buttonBehavior();

	if (ctx->widget.hoveredId == ctx->id)
	{
		ctx->widget.hoveredType = WidgetType::Label;
	}

	return ctx->widget.pressed;
}

}