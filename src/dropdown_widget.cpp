#include <algorithm>
#include "context.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool dropdown(const char* id, i32& selectedIndex, const char** items, u32 itemCount, u32 maxVisibleDropDownItems)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::DropdownBody);
	auto& arrowElem = ctx->theme->getElement(WidgetElementId::DropdownArrow);
	auto& padding = getWidgetPadding();

	ctx->id = genId(id);

	if (ctx->sameLine)
	{
		ctx->widget.customWidth = 80; //TODO: compute based on text size
		ctx->widget.hasCustomWidth = true;
	}

	addWidget((bodyElem.normalState().height + padding.y * 2.0f) * ctx->scale);
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

	ctx->renderer->cmdSetColor(applyTint(bodyElemState->color, TintColorType::Body));
	ctx->renderer->cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->scale);
	ctx->renderer->cmdSetColor(applyTint(arrowElemState->color, TintColorType::Body));

	// dial down the height, since its already global scaled
	auto arrowY = ctx->widget.rect.height / 2.0f - ((arrowElemState->image->height) / 2.0f + (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->scale;

	ctx->renderer->cmdDrawImage(arrowElemState->image,
		{
			ctx->widget.rect.right() - (padding.y + arrowElemState->image->width - (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->scale,
			ctx->widget.rect.top() + arrowY,
			arrowElemState->image->width * ctx->scale,
			arrowElemState->image->height * ctx->scale
		});

	const char* selectedItemText = nullptr;

	if (selectedIndex >= 0 && selectedIndex < itemCount)
	{
		selectedItemText = items[selectedIndex];
	}

	// add the border of the body element
	ctx->widget.rect.x += bodyElemState->border * ctx->scale;

	const auto& popupPos = ctx->widget.rect.bottomLeft();

	if (selectedItemText)
	{
		auto textRc = Rect {
				ctx->widget.rect.x + padding.x * ctx->scale,
				ctx->widget.rect.y,
				ctx->widget.rect.width - ((padding.x + bodyElemState->border) * 2.0f + arrowElemState->image->width) * ctx->scale,
				ctx->widget.rect.height
		};

		ctx->renderer->cmdSetColor(applyTint(bodyElemState->textColor, TintColorType::Text));
		ctx->renderer->cmdSetFont(bodyElemState->font);
		ctx->renderer->cmdDrawTextInBox(
			selectedItemText,
			textRc,
			HAlignType::Left,
			VAlignType::Center, true);
	}

	setFocusable();

	if (ctx->widget.clicked)
	{
		ctx->dropdown.active = !ctx->dropdown.active;

		auto& bodyElem = ctx->theme->getElement(WidgetElementId::SelectableBody);
		ctx->dropDownScrollViewPos = selectedIndex * bodyElem.normalState().height * ctx->scale;

		if (ctx->dropdown.active)
		{
			ctx->dropdown.id = ctx->id;
		}
		else
		{
			ctx->dropdown.id = 0;
		}
	}

	bool selectedNewItem = false;

	if (ctx->dropdown.active && ctx->id == ctx->dropdown.id)
	{
		auto& bodyElem = ctx->theme->getElement(WidgetElementId::DropdownBody);

		// we need exact width, so don't scale the popup's width
		ctx->popupUseGlobalScale = false;

		beginPopup("popup", ctx->widget.rect.width - bodyElem.normalState().border * 2.0f,
			PopupFlags::CustomPosition,
			popupPos,
			WidgetElementId::ButtonBody);

		auto& selectableBodyElem = ctx->theme->getElement(WidgetElementId::SelectableBody).normalState();

		pushSpacing(0);
		pushPadding(PaddingType::ScrollView, Point());

		if (maxVisibleDropDownItems < itemCount)
		{
			pushId(ctx->id);
			beginScrollView("dropDownScrollView", std::min(itemCount, maxVisibleDropDownItems) * selectableBodyElem.height, ctx->dropDownScrollViewPos);
		}

		// we don't want tinting for items, just the dropdown is tinted
		pushTint(Color::white);

		for (u32 i = 0; i < itemCount; i++)
		{
			auto selectedFlag = i == selectedIndex ? SelectableFlags::Selected : SelectableFlags::Normal;

			if (selectable(items[i], selectedFlag))
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
			popId();
		}

		popSpacing();
		popPadding(PaddingType::ScrollView);

		if (selectedNewItem || mustClosePopup())
		{
			closePopup();
			ctx->dropdown.active = false;
		}

		endPopup();
		ctx->popupUseGlobalScale = true;
	}

	return selectedNewItem;
}

}