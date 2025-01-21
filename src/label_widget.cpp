#include "context.h"
#include "theme.h"
#include "font.h"
#include "unicode_text_cache.h"
#include "util.h"

namespace hui
{
bool labelInternal(const char* label, HAlignType horizontalAlign, Font* font)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::LabelBody);
	f32 width = ctx->layoutStack.back().width;
	f32 height = 0;
	auto& bodyElemState = bodyElem.normalState();

	height = bodyElemState.height;

	Rect rect = {
		ctx->position.x,
		ctx->position.y,
		width, height };

	addWidgetItem(label, rect.height * ctx->scale);
	buttonBehavior();

	if (ctx->widget.hoveredWidgetId == ctx->currentWidgetId)
	{
		ctx->widget.hoveredWidgetType = WidgetType::Label;
	}

	if (ctx->widget.visible)
	{
		ctx->renderer->cmdSetColor(tintColor(bodyElemState.textColor, TintColorType::Text));
		ctx->renderer->cmdSetFont(font ? font : bodyElemState.font);
		ctx->renderer->cmdDrawTextInBox(
			ctx->widgetLabel.c_str(),
			ctx->widget.rect,
			horizontalAlign,
			VAlignType::Center);
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

	ctx->renderer->cmdSetColor(tintColor(bodyElemState.textColor, TintColorType::Text));
	ctx->renderer->cmdSetFont((Font*)font);

	f32 width = ctx->layoutStack.back().width;

	ctx->extractLabelAndId(label, ctx->widgetLabel, ctx->currentWidgetId);

	auto textSize = ((Font*)font)->computeTextSize(ctx->widgetLabel.c_str());

	addWidgetItem(label, textSize.height);

	ctx->drawMultilineText(
		ctx->widgetLabel.c_str(),
		{
			ctx->position.x,
			ctx->position.y,
			width,
			0
		},
		horizontalAlign,
		VAlignType::Center);

	buttonBehavior();

	if (ctx->widget.hoveredWidgetId == ctx->currentWidgetId)
	{
		ctx->widget.hoveredWidgetType = WidgetType::Label;
	}

	return ctx->widget.pressed;
}

}