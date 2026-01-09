#include "context.h"
#include "theme.h"
#include "font.h"
#include "unicode_text_cache.h"
#include "util.h"

namespace hui
{
bool list(const char* id, bool* selectedItems, ListSelectionMode selectionType, const char** items, u32 itemCount, f32 height)
{
	if (!items || itemCount == 0 || !selectedItems)
		return false;

	// Determine item height for scroll view calculation
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::SelectableBody);
	auto& bodyElemState = bodyElem.normalState();
	Font* fnt = bodyElemState.font;
	f32 itemHeight = fmaxf(bodyElemState.height, fnt->getMetrics().height) * ctx->scale;

	f32 totalHeight = itemHeight * itemCount;
	f32 scrollPos = 0;
	
	// Generate an ID for storage
	u32 listId = genId(id);
	ctx->id = listId;

	auto iter = ctx->widgetScrollStates.find(listId);
	if (iter != ctx->widgetScrollStates.end())
		scrollPos = iter->second;

	// Use specific height if set, otherwise fill available space or use default
	f32 widgetHeight = height;

	if (height <= 0)
	{
		if (widgetHeight == 0)
			widgetHeight = 200; // Default fallback
	}

	beginScrollView(widgetHeight, scrollPos, totalHeight);

	bool changed = false;

	// We can optimize by only drawing visible items, but for now simple loop
	// ScrollView clip rect handles the actual clipping drawing-wise.
	// beginScrollView sets up the clip.

	// Optimization: Skip items above the view
	// scrollPos is the offset.
	// visible range is [scrollPos, scrollPos + widgetHeight]
	
	// However, we must call `selectable` for layout to work correctly if we are in a vertical layout?
	// `selectable` calls `addWidget` which advances layout.
	// If we skip `selectable`, we must manually advance spacing/position?
	// Actually `beginScrollView` pushes a new layout `LayoutType::ScrollView`.
	// Inside it, `addWidget` just adds to the current Y in the scroll layout.
	// So if we skip calls, the Y won't advance.
	// We can use `beginVirtualListContent` or just manually add space for skipped items.
	
	// Let's try to be simple first: render everything. `selectable` is cheap if clipped?
	// `selectable` does `renderer->cmdDraw...`
	// `renderer` usually has clipping check.

	for (u32 i = 0; i < itemCount; i++)
	{
		bool isSelected = selectedItems[i];

		if (selectable(items[i], isSelected ? SelectableFlags::Selected : SelectableFlags::Normal))
		{
			if (selectionType == ListSelectionMode::Single)
			{
				if (isSelected)
				{
					selectedItems[i] = false;
				}
				else
				{
					// Unselect all others
					for (u32 j = 0; j < itemCount; j++)
						selectedItems[j] = false;

					selectedItems[i] = true;
				}
				changed = true;
			}
			else
			{
				// Toggle selection for multiple
				selectedItems[i] = !selectedItems[i];
				changed = true;
			}
		}
	}

	ctx->widgetScrollStates[listId] = endScrollView();

	return changed;
}

bool selectableInternal(const char* label, HFont font, SelectableFlags stateFlags)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::SelectableBody);
	Font* fnt = font ? (Font*)font : bodyElem.normalState().font;

	ctx->extractLabelAndId(label);
	addWidget(fmaxf(
		bodyElem.normalState().height,
		fnt->getMetrics().height) * ctx->scale);
	buttonBehavior();

	auto bodyElemState = &bodyElem.normalState();

	if (ctx->widget.pressed || ((u32)stateFlags & (u32)SelectableFlags::Selected))
		bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);
	else if (ctx->widget.hovered)
		bodyElemState = &bodyElem.getState(WidgetStateType::Hovered);

	if (ctx->widget.visible)
	{
		ctx->renderer->cmdSetColor(applyTint(bodyElemState->color, TintColorType::Body));
		ctx->renderer->cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->scale);
		ctx->renderer->cmdSetColor(applyTint(bodyElemState->textColor, TintColorType::Text));
		ctx->renderer->cmdSetFont(fnt);
		ctx->renderer->cmdDrawTextInBox(
			ctx->widgetLabel.c_str(),
			Rect(
				ctx->widget.rect.x + bodyElemState->border,
				ctx->widget.rect.y + bodyElemState->border,
				ctx->widget.rect.width - bodyElemState->border * 2,
				ctx->widget.rect.height - bodyElemState->border * 2)
			,
			HAlignType::Left,
			VAlignType::Center, true);
	}

	setFocusable();
	ctx->menuItemTextWidth = fnt->computeTextSize(ctx->widgetLabel.c_str()).width + bodyElemState->border * 2.0f;

	return ctx->widget.clicked;
}

bool selectable(const char* label, SelectableFlags stateFlags)
{
	return selectableInternal(label, nullptr, stateFlags);
}

bool selectableCustomFont(const char* label, HFont font, SelectableFlags stateFlags)
{
	return selectableInternal(label, font, stateFlags);
}

}
