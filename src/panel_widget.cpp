#include "context.h"
#include "theme.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool panel(const char* label, bool* expandedVar)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::PanelBody);
	auto& panelCollapsedArrow = ctx->theme->getElement(WidgetElementId::PanelCollapsedArrow);
	auto& panelExpandedArrow = ctx->theme->getElement(WidgetElementId::PanelExpandedArrow);
	auto bodyElemState = &bodyElem.normalState();
	bool changed = false;
	bool expanded = false;

	addWidgetItem(label, bodyElemState->image->rect.height * ctx->globalScale);

	// we want to have the panel all the way
	ctx->widget.rect.x = ctx->penPosition.x;
	ctx->widget.rect.width = ctx->layoutStack.back().width;

	buttonBehavior();

	if (ctx->widget.clicked)
	{
		if (expandedVar)
		{
			*expandedVar = !*expandedVar;
			changed = true;
			expanded = *expandedVar;
		}
		else
		{
			auto& wvs = ctx->widgetBoolState[ctx->currentWidgetId];
			wvs.lastUsedFrame = ctx->frameCount;
			wvs.value = (f32)!(bool)wvs.value;
			expanded = (bool)wvs.value;
			changed = true;
		}
	}
	else
	{
		if (expandedVar)
		{
			expanded = *expandedVar;
		}
		else
		{
			auto& wvs = ctx->widgetBoolState[ctx->currentWidgetId];
			wvs.lastUsedFrame = ctx->frameCount;
			expanded = (bool)wvs.value;
		}
	}

	if (expanded)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);
	}

	ctx->renderer->cmdSetColor(tintColor(bodyElemState->color, TintColorType::Body));
	ctx->renderer->cmdDrawImageBordered(
		bodyElemState->image,
		bodyElemState->border, ctx->widget.rect, ctx->globalScale);

	auto arrowElemState = &panelCollapsedArrow.normalState();

	// draw arrow
	if (expanded)
	{
		arrowElemState = &panelExpandedArrow.normalState();
	}

	ctx->renderer->cmdSetColor(tintColor(arrowElemState->color, TintColorType::Body));
	ctx->renderer->cmdDrawImage(
		arrowElemState->image,
		{
			round(ctx->widget.rect.x + bodyElemState->border * ctx->globalScale),
			round(ctx->widget.rect.y + (ctx->widget.rect.height - arrowElemState->image->rect.height * ctx->globalScale) / 2.0f),
			arrowElemState->image->rect.width * ctx->globalScale,
			arrowElemState->image->rect.height * ctx->globalScale
		});

	ctx->renderer->cmdSetFont(bodyElemState->font);

	Rect textRect = {
		ctx->widget.rect.x + bodyElemState->border * ctx->globalScale + arrowElemState->image->rect.width * ctx->globalScale,
		ctx->widget.rect.y,
		ctx->widget.rect.width - bodyElemState->border * ctx->globalScale * 2.0f,
		ctx->widget.rect.height
	};

	ctx->renderer->pushClipRect(textRect);
	ctx->renderer->cmdSetColor(tintColor(bodyElemState->textColor, TintColorType::Text));
	ctx->renderer->cmdDrawTextInBox(
		ctx->widgetLabel.c_str(),
		textRect,
		HAlignType::Left, VAlignType::Center);
	ctx->renderer->popClipRect();
	ctx->widget.changeEnded = changed;

	return expanded;
}

}