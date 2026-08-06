#include <algorithm>
#include <unordered_map>
#include "context.h"
#include "theme.h"
#include "font.h"
#include "util.h"
#include "horus.h"

namespace hui
{
bool list(const char* id, bool* selectedItems, ListSelectionMode selectionType, const char** items, u32 itemCount, f32 height, u32 dragDropUserType)
{
	if ((itemCount > 0 && !items) || !selectedItems)
		return false;

	// determine item height for scroll view calculation
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::SelectableBody);
	auto& bodyElemState = bodyElem.normalState();
	Font* fnt = bodyElemState.font;
	if (!fnt)
	{
		fnt = (Font*)hui::themeFontGetFromTheme(ctx->theme, "normal");
		if (!fnt && !ctx->theme->fonts.empty())
			fnt = &ctx->theme->fonts[0]->font;
	}
	f32 itemHeight = fmaxf(bodyElemState.height, fnt ? fnt->getMetrics().height : 16.0f) * ctx->scale;

	f32 totalHeight = itemHeight * itemCount;
	Point scrollOffset;

	u32 listId = genId(id);
		
	ctx->id = listId;

	auto iter = ctx->scrollViewState.find(listId);
	
	if (iter != ctx->scrollViewState.end())
		scrollOffset = iter->second.scrollOffset;

	// use specific height if set, otherwise fill available space or use default
	f32 widgetHeight = height;

	if (height <= 0)
	{
		widgetHeight = 200; // default fallback
	}

	paddingPush(PaddingType::Layout, Point(0, 0));
	paddingPush(PaddingType::ScrollView, Point(0, 0));
	idPush(listId);
	scrollViewBegin("listScrollView", widgetHeight, scrollOffset.y, totalHeight, ScrollViewFlags::NoHorizontalScroll);

	Rect listRect = ctx->scrollViewState[ctx->id].rect;
	bool changed = false;

	// ensure anchor state exists
	if (ctx->listAnchors.find(listId) == ctx->listAnchors.end())
		ctx->listAnchors[listId] = -1;

	i32& anchor = ctx->listAnchors[listId];

	if (ctx->isActiveLayer() && listRect.contains(ctx->mousePosition))
	{
		if (ctx->event.type == InputEvent::Type::Key && ctx->event.key.down)
		{
			if (ctx->event.key.code == KeyCode::Esc)
			{
				for (u32 i = 0; i < itemCount; i++) selectedItems[i] = false;
				changed = true;
				anchor = -1;
				inputEventCancel();
			}
			else if (selectionType == ListSelectionMode::Multiple
				&& ctx->event.key.code == KeyCode::A
				&& (has(ctx->event.key.modifiers, KeyModifiers::Control)))
			{
				for (u32 i = 0; i < itemCount; i++) selectedItems[i] = true;
				changed = true;
				inputEventCancel();
			}
		}
	}
	spacingPush(0.0f);
	i32 hoveredItem = -1;
	for (u32 i = 0; i < itemCount; i++)
	{
		bool isSelected = selectedItems[i];
		bool itemClicked = selectable(items[i], isSelected ? SelectableFlags::Selected : SelectableFlags::Normal);

		if (ctx->widget.hovered)
			hoveredItem = (i32)i;

		if (itemClicked)
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
					// unselect all others
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
					// range selection
					// if Ctrl is NOT pressed, clear selection first
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
					// toggle selection for multiple
					selectedItems[i] = !selectedItems[i];
					anchor = i;
				}
				else
				{
					// normal click: Select only this
					for (u32 j = 0; j < itemCount; j++) selectedItems[j] = false;

					selectedItems[i] = true;
					anchor = i;
				}
				
				changed = true;
			}
		}
	}

	spacingPop();

	if (dragDropUserType > 0)
	{
		u32 savedId = ctx->id;
		ctx->id = listId;

		ctx->widget.hovered = ctx->hoveringThisWindow && listRect.contains(ctx->mousePosition);

		if (dragDropWantsTo() && hoveredItem >= 0)
			dragDropBegin(dragDropUserType, (void*)items[hoveredItem]);

		if (dragDropGetObjectType() == dragDropUserType)
		{
			bool isSourceList = ctx->dragDrop.dragging && ctx->dragDrop.id == listId;
			if (!isSourceList)
			{
				bool isSelf = false;
				for (u32 j = 0; j < itemCount; j++)
					if (items[j] == (const char*)dragDropGetObject()) { isSelf = true; break; }
				if (!isSelf)
					dragDropAllow();
			}
		}

		ctx->id = savedId;
	}

	ctx->scrollViewState[listId].scrollOffset = scrollViewEnd();
	
	auto listState = ctx->widget;
	
	idPop();
	paddingPop(PaddingType::ScrollView);
	paddingPop(PaddingType::Layout);
	ctx->id = listId;
	ctx->widget = listState;

	return changed;
}

bool selectableInternal(const char* label, HFont font, SelectableFlags stateFlags)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::SelectableBody);
	Font* fnt = font ? (Font*)font : bodyElem.normalState().font;

	if (!fnt)
	{
		fnt = (Font*)hui::themeFontGetFromTheme(ctx->theme, "normal");
		if (!fnt && !ctx->theme->fonts.empty())
			fnt = &ctx->theme->fonts[0]->font;
	}

	ctx->setLabelAndId(label);
	addWidget(fmaxf(
		bodyElem.normalState().height,
		fnt ? fnt->getMetrics().height : 16.0f) * ctx->scale);
	buttonBehavior();

	auto bodyElemState = &bodyElem.normalState();

	if (ctx->widget.disabled)
		bodyElemState = &bodyElem.disabledState();
	else if (ctx->widget.hovered)
		bodyElemState = &bodyElem.getState(WidgetStateType::Hovered);
	else if (ctx->widget.pressed || ((u32)stateFlags & (u32)SelectableFlags::Selected))
		bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);

	if (ctx->widget.visible)
	{
		ctx->renderer.cmdSetColor(tintApply(bodyElemState->color, TintColorType::Body));
		ctx->renderer.cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->scale);
		ctx->renderer.cmdSetColor(tintApply(bodyElemState->textColor, TintColorType::Text));
		if (fnt)
			ctx->renderer.cmdSetFont(fnt);
		ctx->renderer.cmdDrawTextInBox(
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

	widgetSetFocusable();
	if (fnt)
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
