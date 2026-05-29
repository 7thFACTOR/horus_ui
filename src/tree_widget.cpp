#include "context.h"
#include "theme.h"
#include "renderer.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool treeNode(const char* label, bool* expandedVar, SelectableFlags stateFlags)
{
	auto& collapsedArrow = ctx->theme->getElement(WidgetElementId::TreeNodeCollapsedArrow);
	auto& expandedArrow = ctx->theme->getElement(WidgetElementId::TreeNodeExpandedArrow);
	auto collapsedArrowState = &collapsedArrow.normalState();
	auto expandedArrowState = &expandedArrow.normalState();

	bool changed = false;
	bool expanded = false;
	const auto& padding = widgetGetPadding();

	ctx->setLabelAndId(label);
	u32 nodeId = ctx->id;

	if (expandedVar)
	{
		expanded = *expandedVar;
	}
	else
	{
		auto& wvs = ctx->widgetBools[nodeId];
		expanded = (bool)wvs.value;
	}

	auto arrowElemState = expanded ? expandedArrowState : collapsedArrowState;

	if (ctx->widget.disabled)
	{
		arrowElemState = expanded ? &expandedArrow.disabledState() : &collapsedArrow.disabledState();
	}

	f32 arrowWidth = arrowElemState->image ? arrowElemState->image->width : 22.0f;
	f32 arrowHeight = arrowElemState->image ? arrowElemState->image->height : 22.0f;

	addWidget(arrowHeight * ctx->scale);
	buttonBehavior();

	if (ctx->widget.clicked)
	{
		if (expandedVar)
		{
			*expandedVar = !*expandedVar;
			expanded = *expandedVar;
		}
		else
		{
			auto& wvs = ctx->widgetBools[nodeId];
			wvs.lastUsedFrame = ctx->frameCount;
			wvs.value = (f32)!(bool)wvs.value;
			expanded = (bool)wvs.value;
		}
		changed = true;
	}

	if (arrowElemState->image)
	{
		ctx->renderer.cmdSetColor(tintApply(arrowElemState->color, TintColorType::Body));
		ctx->renderer.cmdDrawImage(
			arrowElemState->image,
			{
				ctx->widget.rect.x,
				ctx->widget.rect.y + (ctx->widget.rect.height - arrowHeight * ctx->scale) / 2.0f,
				arrowWidth * ctx->scale,
				arrowHeight * ctx->scale
			});
	}

	sameLine();
	selectable(label, stateFlags);

	return expanded;
}

bool treeNodeBegin(const char* label, bool* expandedVar, SelectableFlags stateFlags)
{
	if (!treeNode(label, expandedVar, stateFlags))
	{
		return false;
	}

	auto& arrowState = ctx->theme->getElement(WidgetElementId::TreeNodeCollapsedArrow).normalState();
	const auto& padding = widgetGetPadding();
	f32 arrowWidth = arrowState.image ? arrowState.image->width : 22.0f;
	f32 indent = (padding.x + arrowWidth) * ctx->scale;

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

void treeNodeEnd()
{
	// Flush any pending sameLine advancement before capturing position,
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
