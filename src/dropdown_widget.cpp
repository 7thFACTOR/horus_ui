#include "horus.h"
#include "types.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "context.h"
#include "util.h"
#include <algorithm>

namespace hui
{
bool dropdown(i32& selectedIndex, const char** items, u32 itemCount, u32 maxVisibleDropDownItems)
{
	auto bodyElem = ctx->theme->getElement(WidgetElementId::DropdownBody);
	auto arrowElem = ctx->theme->getElement(WidgetElementId::DropdownArrow);

	addWidgetItem(bodyElem.normalState().height * ctx->globalScale);
	buttonBehavior();

	auto bodyElemState = &bodyElem.normalState();
	auto arrowElemState = &arrowElem.normalState();

	if (ctx->widget.pressed)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);
		arrowElemState = &arrowElem.getState(WidgetStateType::Pressed);
	}
	else if (ctx->widget.focused)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Focused);
		arrowElemState = &arrowElem.getState(WidgetStateType::Focused);
	}
	else if (ctx->widget.hovered)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Hovered);
		arrowElemState = &arrowElem.getState(WidgetStateType::Hovered);
	}

	ctx->renderer->cmdSetColor(bodyElemState->color * ctx->tint[(int)TintColorType::Body]);
	ctx->renderer->cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->globalScale);
	ctx->renderer->cmdSetColor(arrowElemState->color * ctx->tint[(int)TintColorType::Body]);

	// dial down the height, since its already global scaled
	auto arrowY = ((ctx->widget.rect.height / ctx->globalScale - arrowElemState->image->rect.height) / 2.0f + (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->globalScale;

	ctx->renderer->cmdDrawImage(arrowElemState->image,
		{
			ctx->widget.rect.right() - (arrowElemState->image->rect.width - (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->globalScale,
			ctx->widget.rect.top() + arrowY,
			arrowElemState->image->rect.width * ctx->globalScale,
			arrowElemState->image->rect.height * ctx->globalScale
		});

	const char* selectedItemText = nullptr;

	if (selectedIndex >= 0 && selectedIndex < itemCount)
	{
		selectedItemText = items[selectedIndex];
	}

	// add the border of the body element
	ctx->widget.rect.x += bodyElemState->border * ctx->globalScale;

	auto posForPopup = ctx->widget.rect.bottomLeft();

	if (selectedItemText)
	{
		ctx->renderer->pushClipRect(ctx->widget.rect);
		ctx->renderer->cmdSetColor(bodyElemState->textColor * ctx->tint[(int)TintColorType::Text]);
		ctx->renderer->cmdSetFont(bodyElemState->font);
		ctx->renderer->cmdDrawTextInBox(
			selectedItemText,
			ctx->widget.rect,
			HAlignType::Left,
			VAlignType::Center);
		ctx->renderer->popClipRect();
	}

	setAsFocusable();
	ctx->currentWidgetId++;

	if (ctx->widget.clicked)
	{
		ctx->dropdownState.active = !ctx->dropdownState.active;
		ctx->dropDownScrollViewPos = 0;

		if (ctx->dropdownState.active)
		{
			ctx->dropdownState.widgetId = ctx->currentWidgetId;
		}
		else
		{
			ctx->dropdownState.widgetId = 0;
		}
	}

	bool selectedNewItem = false;

	if (ctx->dropdownState.active && ctx->currentWidgetId == ctx->dropdownState.widgetId)
	{
		auto bodyElem = ctx->theme->getElement(WidgetElementId::DropdownBody);

		// we need exact width, so don't scale the popup's width
		ctx->popupUseGlobalScale = false;

		beginPopup(ctx->widget.rect.width - bodyElem.normalState().border * 2,
			PopupFlags::CustomPosition,
			posForPopup,
			WidgetElementId::ButtonBody);

		auto selectableBodyElem = ctx->theme->getElement(WidgetElementId::SelectableBody).normalState();

		pushSpacing(0);
		pushPadding(0);

		if (maxVisibleDropDownItems < itemCount)
		{
			beginScrollView(std::min(itemCount, maxVisibleDropDownItems) * selectableBodyElem.height, ctx->dropDownScrollViewPos);
		}

		// we don't want tinting for items, just the dropdown is tinted
		pushTint(Color::white);

		for (u32 i = 0; i < itemCount; i++)
		{
			if (selectable(items[i]))
			{
				selectedIndex = i;
				selectedNewItem = true;
				ctx->widget.changeEnded = true;
			}
		}

		popTint();

		if (maxVisibleDropDownItems < itemCount)
		{
			ctx->dropDownScrollViewPos = endScrollView();
		}

		popSpacing();
		popPadding();

		if (selectedNewItem || mustClosePopup())
		{
			closePopup();
			ctx->dropdownState.active = false;
		}

		endPopup();
		ctx->popupUseGlobalScale = true;
	}

	return selectedNewItem;
}

bool dropdown(i32& selectedIndex,
	void* userdata,
	bool(*itemSource)(void* userdata, i32 index, char** outItemText),
	u32 maxVisibleDropDownItems)
{
	return false;
}

}