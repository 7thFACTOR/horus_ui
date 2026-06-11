#include "context.h"
#include "theme.h"
#include "renderer.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool treeNode(const char* label, bool* expandedVar, SelectableFlags stateFlags, TreeNodeFlags treeFlags)
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

	auto& bodyElem = ctx->theme->getElement(WidgetElementId::SelectableBody);
	Font* fnt = bodyElem.normalState().font;
	f32 labelHeight = fmaxf(bodyElem.normalState().height, fnt->getMetrics().height);

	ctx->id = genId(nodeId);
	ctx->widget.nextWidth = arrowWidth;
	ctx->widget.hasNextWidth = true;
	addWidget(labelHeight * ctx->scale);
	buttonBehavior();

	bool arrowClicked = ctx->widget.clicked;

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
	bool labelClicked = selectable(label, stateFlags);
	bool labelDoubleClicked = labelClicked && (ctx->event.mouse.clickCount == 2);
	bool labelToggleClicked = !!(treeFlags & TreeNodeFlags::ToggleOnSelect)
		? labelClicked && ctx->event.mouse.clickCount == 1
		: labelDoubleClicked;

	if (arrowClicked || labelToggleClicked)
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

	// Flush sameLine row before returning to ensure layout/indentation for children works correctly
	if (!ctx->sameLine.enabled && ctx->sameLine.wasEnabled)
	{
		ctx->position.x = ctx->sameLine.currentPosition.x;
		ctx->position.y += ctx->sameLine.maxHeight + ctx->spacing * ctx->scale;
		ctx->sameLine.wasEnabled = false;
		ctx->sameLine.maxHeight = 0;
		ctx->sameLine.lastLineWidth = 0;
	}

	return expanded;
}

bool treeNodeBegin(const char* label, bool* expandedVar, SelectableFlags stateFlags, TreeNodeFlags treeFlags)
{
	if (!treeNode(label, expandedVar, stateFlags, treeFlags))
	{
		return false;
	}

	// Ensure any pending sameLine advancement is flushed so the label X is computed correctly.
	// This avoids using a stale widget.rect.x that may still be on a same-line row,
	// which can cause the next sibling (or subsequent nodes) to be indented under the wrong parent.
	if (!ctx->sameLine.enabled && ctx->sameLine.wasEnabled)
	{
		ctx->position.x = ctx->sameLine.currentPosition.x;
		ctx->position.y += ctx->sameLine.maxHeight + ctx->spacing * ctx->scale;
		ctx->sameLine.wasEnabled = false;
		ctx->sameLine.maxHeight = 0;
		ctx->sameLine.lastLineWidth = 0;
	}

	auto& arrowState = ctx->theme->getElement(WidgetElementId::TreeNodeCollapsedArrow).normalState();
	f32 arrowWidth = arrowState.image ? arrowState.image->width : 22.0f;
	f32 indent = themeGetWidgetElementParameterFloat(ctx->theme, WidgetElementId::TreeNodeBody, "default", "indent", 20.0f) * ctx->scale;

	f32 labelX = ctx->widget.rect.x;
	if (labelX == 0.0f)
	{
		labelX = ctx->position.x + arrowWidth * ctx->scale + ctx->sameLine.spacing * ctx->scale;
	}

	// The selectable label was placed by treeNode() on the same line after the arrow.
	// Compute the arrow's X position for proper sibling alignment.
	f32 arrowX = labelX - arrowWidth * ctx->scale - ctx->sameLine.spacing * ctx->scale;

	// Push layout, then:
	// 1) update the layout on the stack (the one that will be restored on pop)
	//    so siblings will align to arrowX (parent arrow X).
	// 2) set the active layout's savedPosition.x to labelX + indent for children.
	layoutPush();

	// Update the parent layout stored on the stack so that when we pop back,
	// ctx->layout.savedPosition.x == arrowX (siblings align to arrow X).
	if (!ctx->layoutStack.empty())
	{
		auto& parentLayout = ctx->layoutStack.back();
		f32 oldSavedXStack = parentLayout.savedPosition.x;
		f32 deltaStack = arrowX - oldSavedXStack;
		parentLayout.savedPosition.x = arrowX;
		parentLayout.width -= deltaStack;
		if (parentLayout.width < 0.0f)
		{
			parentLayout.width = 0.0f;
		}
	}

	// Now adjust the current layout (children layout) so children start at labelX + indent.
	f32 oldSavedX = ctx->layout.savedPosition.x;
	f32 newSavedX = labelX + indent;
	f32 delta = newSavedX - oldSavedX;

	ctx->layout.savedPosition.x = newSavedX;
	ctx->layout.width -= delta;

	if (ctx->layout.width < 0.0f)
	{
		ctx->layout.width = 0.0f;
	}
	
	// Keep the drawing cursor at the parent's label X so children start
	// at the parent's label position.
	ctx->position.x = labelX;

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
