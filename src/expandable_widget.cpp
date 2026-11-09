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
	auto& collapsedArrow = ctx->theme->getElement(WidgetElementId::ExpandableCollapsedArrow);
	auto& expandedArrow = ctx->theme->getElement(WidgetElementId::ExpandableExpandedArrow);
	auto bodyElemState = &bodyElem.normalState();
	auto expandedArrowState = &expandedArrow.normalState();
	auto collapsedArrowState = &collapsedArrow.normalState();

	bool changed = false;
	bool expanded = false;
	const auto& padding = widgetGetPadding();

	ctx->setLabelAndId(label);
	f32 widgetHeight = bodyElemState->height > 0 ? bodyElemState->height : bodyElemState->image->rect.height;
	addWidget(widgetHeight + padding.y * 2.0f);
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

	if (ctx->widget.disabled)
	{
		bodyElemState = &bodyElem.disabledState();
		expandedArrowState = &expandedArrow.disabledState();
		collapsedArrowState = &collapsedArrow.disabledState();
	}
	else if (expanded)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);
	}
	else if (ctx->widget.focused)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Focused);
	}
	else if (ctx->widget.hovered)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Hovered);
	}

	ctx->renderer.cmdSetColor(tintApply(bodyElemState->color, TintColorType::Body));
	ctx->renderer.cmdDrawImageBordered(
		bodyElemState->image,
		bodyElemState->border, 
		ctx->widget.rect,
		ctx->scale);

	auto arrowElemState = collapsedArrowState;

	// draw arrow
	if (expanded)
	{
		arrowElemState = expandedArrowState;
	}

	ctx->renderer.cmdSetColor(tintApply(arrowElemState->color, TintColorType::Body));
	ctx->renderer.cmdDrawImage(
		arrowElemState->image,
		{
			ctx->widget.rect.x + (padding.x + bodyElemState->border) * ctx->scale,
			ctx->widget.rect.y + (ctx->widget.rect.height - arrowElemState->image->height * ctx->scale) / 2.0f,
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

bool expandableBegin(const char* label, bool* expandedVar)
{
	if (!expandable(label, expandedVar))
	{
		return false;
	}

	auto& bodyState = ctx->theme->getElement(WidgetElementId::ExpandableBody).normalState();
	auto& arrowState = ctx->theme->getElement(WidgetElementId::ExpandableCollapsedArrow).normalState();
	const auto& padding = widgetGetPadding();
	f32 arrowWidth = arrowState.image ? arrowState.image->width : bodyState.height;
	f32 indent = (bodyState.border + padding.x + arrowWidth) * ctx->scale;

	layoutPush();
	ctx->layout.savedPosition.x += indent;
	ctx->layout.width -= indent;

	if (ctx->layout.width < 0.0f)
	{
		ctx->layout.width = 0.0f;
	}
	
	ctx->position.x += indent;

	return true;
}

void expandableEnd()
{
	// flush any pending sameLine advancement before capturing position,
	// otherwise the Y stays at the top of the sameLine row
	if (!ctx->sameLine.enabled && ctx->sameLine.wasEnabled)
	{
		ctx->position.y += ctx->sameLine.maxHeight + ctx->spacing * ctx->scale;
		ctx->sameLine.wasEnabled = false;
		ctx->sameLine.maxHeight = 0;
	}

	Point pos = ctx->position;
	
	layoutPop();
	ctx->position = pos;
	ctx->position.x = ctx->layout.savedPosition.x;
}

}
