#include "horus.h"
#include "types.h"
#include "theme.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "context.h"
#include "util.h"

namespace hui
{
bool panel(const char* labelText, bool expanded)
{
	auto bodyElem = ctx->theme->getElement(WidgetElementId::PanelBody);
	auto panelCollapsedArrow = ctx->theme->getElement(WidgetElementId::PanelCollapsedArrow);
	auto panelExpandedArrow = ctx->theme->getElement(WidgetElementId::PanelExpandedArrow);
	auto bodyElemState = &bodyElem.normalState();

	addWidgetItem(bodyElemState->image->rect.height * ctx->globalScale);

	// we want to have the panel all the way
	ctx->widget.rect.x = ctx->penPosition.x;
	ctx->widget.rect.width = ctx->layoutStack.back().width;

	buttonBehavior();

	if (ctx->widget.clicked)
	{
		expanded = !expanded;
	}

	if (expanded)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);
	}

	ctx->renderer->cmdSetColor(bodyElemState->color);
	ctx->renderer->cmdDrawImageBordered(
		bodyElemState->image,
		bodyElemState->border, ctx->widget.rect, ctx->globalScale);

	auto arrowElemState = &panelCollapsedArrow.normalState();

	// draw arrow
	if (expanded)
	{
		arrowElemState = &panelExpandedArrow.normalState();
	}

	ctx->renderer->cmdSetColor(arrowElemState->color);
	ctx->renderer->cmdDrawImage(
		arrowElemState->image,
		{
			round(ctx->widget.rect.x + bodyElemState->border * ctx->globalScale),
			round(ctx->widget.rect.y + (ctx->widget.rect.height - arrowElemState->image->rect.height * ctx->globalScale) / 2.0f),
			arrowElemState->image->rect.width * ctx->globalScale,
			arrowElemState->image->rect.height * ctx->globalScale
		});

	ctx->renderer->cmdSetColor(bodyElemState->textColor);
	ctx->renderer->cmdSetFont(bodyElemState->font);

	Rect textRect = {
		ctx->widget.rect.x + bodyElemState->border * ctx->globalScale + arrowElemState->image->rect.width * ctx->globalScale,
		ctx->widget.rect.y,
		ctx->widget.rect.width - bodyElemState->border * ctx->globalScale * 2.0f,
		ctx->widget.rect.height
	};

	ctx->renderer->pushClipRect(textRect);
	ctx->renderer->cmdSetColor(bodyElemState->textColor * ctx->tint[(u32)TintColorType::Text]);
	ctx->renderer->cmdDrawTextInBox(
		labelText,
		textRect,
		HAlignType::Left, VAlignType::Center);
	ctx->renderer->popClipRect();
	ctx->currentWidgetId++;

	return expanded;
}

}