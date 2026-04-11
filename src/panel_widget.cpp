#include "context.h"
#include "theme.h"
#include "renderer.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool expandable(const char* label, bool* expandedVar)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::ExpandableBody);
	auto& panelCollapsedArrow = ctx->theme->getElement(WidgetElementId::ExpandableCollapsedArrow);
	auto& panelExpandedArrow = ctx->theme->getElement(WidgetElementId::ExpandableExpandedArrow);
	auto bodyElemState = &bodyElem.normalState();
	bool changed = false;
	bool expanded = false;
	const auto& padding = widgetGetPadding();

	ctx->labelAndIdSet(label);
	widgetAdd((bodyElemState->image->rect.height + padding.y * 2.0f) * ctx->scale);
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
			auto& wvs = ctx->widgetBools[ctx->id];
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
			auto& wvs = ctx->widgetBools[ctx->id];
			wvs.lastUsedFrame = ctx->frameCount;
			expanded = (bool)wvs.value;
		}
	}

	if (expanded)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);
	}

	ctx->renderer.cmdSetColor(tintApply(bodyElemState->color, TintColorType::Body));
	ctx->renderer.cmdDrawImageBordered(
		bodyElemState->image,
		bodyElemState->border, 
		ctx->widget.rect,
		ctx->scale);

	auto arrowElemState = &panelCollapsedArrow.normalState();

	// draw arrow
	if (expanded)
	{
		arrowElemState = &panelExpandedArrow.normalState();
	}

	ctx->renderer.cmdSetColor(tintApply(arrowElemState->color, TintColorType::Body));
	ctx->renderer.cmdDrawImage(
		arrowElemState->image,
		{
			round(ctx->widget.rect.x + (padding.x + bodyElemState->border) * ctx->scale),
			round(ctx->widget.rect.y + (ctx->widget.rect.height - arrowElemState->image->height * ctx->scale) / 2.0f),
			arrowElemState->image->width * ctx->scale,
			arrowElemState->image->height * ctx->scale
		});

	ctx->renderer.cmdSetFont(bodyElemState->font);

	Rect textRect = {
		ctx->widget.rect.x + (bodyElemState->border + arrowElemState->image->width + padding.x) * ctx->scale,
		ctx->widget.rect.y,
		ctx->widget.rect.width - (padding.x + bodyElemState->border) * 2.0f * ctx->scale,
		ctx->widget.rect.height
	};

	ctx->renderer.pushClipRect(textRect);
	ctx->renderer.cmdSetColor(tintApply(bodyElemState->textColor, TintColorType::Text));
	ctx->renderer.cmdDrawTextInBox(
		ctx->widgetLabel.c_str(),
		textRect,
		HAlignType::Left, VAlignType::Center);
	ctx->renderer.popClipRect();
	ctx->widget.changeEnded = changed;

	return expanded;
}

}