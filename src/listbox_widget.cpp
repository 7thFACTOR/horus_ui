#include <algorithm>
#include <unordered_map>
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

	pushPadding(PaddingType::Layout, Point(0, 0));
	pushPadding(PaddingType::ScrollView, Point(0, 0));
	pushId(listId);
	beginScrollView("listScrollView", widgetHeight, scrollPos, totalHeight);

	Rect viewRect = ctx->scrollViewStack[ctx->scrollViewDepth - 1].rect;
	bool changed = false;

	// Ensure anchor state exists
	if (ctx->listAnchors.find(listId) == ctx->listAnchors.end())
		ctx->listAnchors[listId] = -1;

	i32& anchor = ctx->listAnchors[listId];

	if (ctx->isActiveLayer() && viewRect.contains(ctx->mousePosition))
	{
		if (ctx->event.type == InputEvent::Type::Key && ctx->event.key.down)
		{
			if (ctx->event.key.code == KeyCode::Esc)
			{
				for (u32 i = 0; i < itemCount; i++) selectedItems[i] = false;
				changed = true;
				anchor = -1;
				cancelEvent();
			}
			else if (selectionType == ListSelectionMode::Multiple
				&& ctx->event.key.code == KeyCode::A
				&& (has(ctx->event.key.modifiers, KeyModifiers::Control)))
			{
				for (u32 i = 0; i < itemCount; i++) selectedItems[i] = true;
				changed = true;
				cancelEvent();
			}
		}
	}

	for (u32 i = 0; i < itemCount; i++)
	{
		bool isSelected = selectedItems[i];

		if (selectable(items[i], isSelected ? SelectableFlags::Selected : SelectableFlags::Normal))
		{
			auto mods = ctx->event.mouse.modifiers;

			if (selectionType == ListSelectionMode::Single)
			{
				if (isSelected)
				{
					selectedItems[i] = false;
					anchor = -1;
				}
				else
				{
					// Unselect all others
					for (u32 j = 0; j < itemCount; j++)
						selectedItems[j] = false;

					selectedItems[i] = true;
					anchor = i;
				}
				changed = true;
			}
			else
			{
				if (has(mods, KeyModifiers::Shift) && anchor != -1)
				{
					// Range selection
					// If Ctrl is NOT pressed, clear selection first
					if (!(mods & KeyModifiers::Control))
					{
						for (u32 j = 0; j < itemCount; j++) selectedItems[j] = false;
					}

					u32 start = std::min((u32)anchor, i);
					u32 end = std::max((u32)anchor, i);

					for (u32 j = start; j <= end; j++) selectedItems[j] = true;
				}
				else if (has(mods, KeyModifiers::Control))
				{
					// Toggle selection for multiple
					selectedItems[i] = !selectedItems[i];
					anchor = i;
				}
				else
				{
					// Normal click: Select only this
					for (u32 j = 0; j < itemCount; j++) selectedItems[j] = false;
					selectedItems[i] = true;
					anchor = i;
				}
				changed = true;
			}
		}
	}

	ctx->widgetScrollStates[listId] = endScrollView();
	popId();
	popPadding(PaddingType::ScrollView);
	popPadding(PaddingType::Layout);

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

	if (ctx->widget.hovered)
		bodyElemState = &bodyElem.getState(WidgetStateType::Hovered);
	else if (ctx->widget.pressed || ((u32)stateFlags & (u32)SelectableFlags::Selected))
		bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);

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
