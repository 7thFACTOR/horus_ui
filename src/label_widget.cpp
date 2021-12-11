#include "horus.h"
#include "types.h"
#include "theme.h"
#include "font.h"
#include "context.h"
#include "unicode_text_cache.h"
#include "util.h"

namespace hui
{
bool labelInternal(const char* labelText, HAlignType horizontalAlign, UiFont* font)
{
	auto bodyElem = ctx->theme->getElement(WidgetElementId::LabelBody);
	f32 width = ctx->layoutStack.back().width - ctx->padding * 2.0f;
	f32 height = 0;
	auto bodyElemState = bodyElem.normalState();

	height = bodyElemState.height;

	Rect rect = {
		ctx->penPosition.x + ctx->padding,
		ctx->penPosition.y,
		width, height };

	addWidgetItem(rect.height * ctx->globalScale);
	buttonBehavior();

	if (ctx->widget.hoveredWidgetId == ctx->currentWidgetId)
	{
		ctx->widget.hoveredWidgetType = WidgetType::Label;
	}

	if (ctx->widget.visible)
	{
		ctx->renderer->cmdSetColor(bodyElemState.textColor * ctx->tint[(int)TintColorType::Text]);
		ctx->renderer->cmdSetFont(font ? font : bodyElemState.font);
		ctx->renderer->cmdDrawTextInBox(
			labelText,
			ctx->widget.rect,
			horizontalAlign,
			VAlignType::Center);
	}

	ctx->currentWidgetId++;

	return ctx->widget.clicked;
}

bool label(const char* labelText, HAlignType horizontalAlign)
{
	return labelInternal(labelText, horizontalAlign, nullptr);
}

bool labelCustomFont(const char* labelText, HFont font, HAlignType horizontalAlign)
{
	return labelInternal(labelText, horizontalAlign, (UiFont*)font);
}

bool multilineLabel(const char* labelText, HAlignType horizontalAlign)
{
	auto bodyElemState = ctx->theme->getElement(WidgetElementId::LabelBody).normalState();

	return multilineLabelCustomFont(labelText, bodyElemState.font, horizontalAlign);
}

bool multilineLabelCustomFont(const char* labelText, HFont font, HAlignType horizontalAlign)
{
	auto bodyElemState = ctx->theme->getElement(WidgetElementId::LabelBody).normalState();

	ctx->renderer->cmdSetColor(bodyElemState.textColor * ctx->tint[(int)TintColorType::Text]);
	ctx->renderer->cmdSetFont((UiFont*)font);

	f32 width = ctx->layoutStack.back().width - ctx->padding * 2.0f * ctx->globalScale;

	auto rect = ctx->drawMultilineText(
		labelText,
		{
			ctx->penPosition.x + ctx->padding * ctx->globalScale,
			ctx->penPosition.y,
			width,
			0 },
			horizontalAlign,
			VAlignType::Center);

	addWidgetItem(rect.height);
	buttonBehavior();

	if (ctx->widget.hoveredWidgetId == ctx->currentWidgetId)
	{
		ctx->widget.hoveredWidgetType = WidgetType::Label;
	}

	ctx->currentWidgetId++;

	return ctx->widget.pressed;
}

}