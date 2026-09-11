#include <algorithm>
#include "context.h"
#include "theme.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool dropdown(const char* id, i32& selectedIndex, const char** items, u32 itemCount, u32 maxVisibleDropDownItems, const char* indeterminate)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::DropdownBody);
	auto& arrowBoxElem = ctx->theme->getElement(WidgetElementId::DropdownArrowBox);
	auto& arrowElem = ctx->theme->getElement(WidgetElementId::DropdownArrow);
	auto& padding = widgetGetPadding();

	ctx->id = genId(id);

	if (ctx->sameLine.enabled)
	{
		ctx->widget.customWidth = 80; //TODO: compute based on text size
		ctx->widget.hasCustomWidth = true;
	}

	addWidget(bodyElem.normalState().height + padding.y * 2.0f);
	buttonBehavior();

	if (ctx->dropdown.active && ctx->widget.captureId == ctx->id)
	{
		// the open popup closes itself as an outside click, so don't let the
		// body capture the press, otherwise the mouse release would re-open it
		ctx->widget.captureId = 0;
	}

	auto bodyElemState = &bodyElem.normalState();
	auto arrowBoxElemState = &arrowBoxElem.normalState();
	auto arrowElemState = &arrowElem.normalState();

	if (ctx->widget.disabled)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Disabled);
		arrowBoxElemState = &arrowBoxElem.getState(WidgetStateType::Disabled);
		arrowElemState = &arrowElem.getState(WidgetStateType::Disabled);
	}
	else if (ctx->widget.pressed)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);
		arrowBoxElemState = &arrowBoxElem.getState(WidgetStateType::Pressed);
		arrowElemState = &arrowElem.getState(WidgetStateType::Pressed);
	}
	else if (ctx->widget.focused)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Focused);
		arrowBoxElemState = &arrowBoxElem.getState(WidgetStateType::Focused);
		arrowElemState = &arrowElem.getState(WidgetStateType::Focused);
	}
	else if (ctx->widget.hovered)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Hovered);
		arrowBoxElemState = &arrowBoxElem.getState(WidgetStateType::Hovered);
		arrowElemState = &arrowElem.getState(WidgetStateType::Hovered);
	}

	// calculate arrow box rect (right side of dropdown)
	f32 arrowBoxWidth = arrowBoxElemState->width * ctx->scale;
	Rect arrowBoxRect = {
		ctx->widget.rect.right() - arrowBoxWidth,
		ctx->widget.rect.y,
		arrowBoxWidth,
		ctx->widget.rect.height
	};

	// calculate body rect (left portion, excluding arrow box)
	Rect bodyRect = {
		ctx->widget.rect.x,
		ctx->widget.rect.y,
		ctx->widget.rect.width - arrowBoxWidth,
		ctx->widget.rect.height
	};

	// draw body background
	ctx->renderer.cmdSetColor(tintApply(bodyElemState->color, TintColorType::Body));
	ctx->renderer.cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, bodyRect, ctx->scale);

	// draw arrow box background
	ctx->renderer.cmdSetColor(tintApply(arrowBoxElemState->color, TintColorType::Body));
	ctx->renderer.cmdDrawImageBordered(arrowBoxElemState->image, arrowBoxElemState->border, arrowBoxRect, ctx->scale);

	// draw arrow image centered in arrow box
	ctx->renderer.cmdSetColor(tintApply(arrowElemState->color, TintColorType::Body));
	auto arrowY = arrowBoxRect.y + arrowBoxRect.height / 2.0f - (arrowElemState->image->height / 2.0f) * ctx->scale;
	auto arrowX = arrowBoxRect.x + arrowBoxRect.width / 2.0f - (arrowElemState->image->width / 2.0f) * ctx->scale;

	ctx->renderer.cmdDrawImage(arrowElemState->image,
		{
			arrowX,
			arrowY,
			arrowElemState->image->width * ctx->scale,
			arrowElemState->image->height * ctx->scale
		});

	const char* selectedItemText = nullptr;

	if (selectedIndex >= 0 && selectedIndex < itemCount)
	{
		selectedItemText = items[selectedIndex];
	}

	// add the border of the body element
	Rect textRect = {
		bodyRect.x + bodyElemState->border * ctx->scale + padding.x * ctx->scale,
		bodyRect.y,
		bodyRect.width - (bodyElemState->border * 2.0f + padding.x) * ctx->scale,
		bodyRect.height
	};

	const auto& popupPos = ctx->widget.rect.bottomLeft();

	if (selectedItemText)
	{
		ctx->renderer.cmdSetColor(tintApply(bodyElemState->textColor, TintColorType::Text));
		ctx->renderer.cmdSetFont(bodyElemState->font);
		ctx->renderer.cmdDrawTextInBox(
			selectedItemText,
			textRect,
			HAlignType::Left,
			VAlignType::Center, true);
	}
	else if (indeterminate)
	{
		// use the text input's default/hint text style so the indeterminate
		// text reads as a hint rather than a real selection
		auto& indeterminateTextElem = ctx->theme->getElement(WidgetElementId::TextInputDefaultText);
		auto& indeterminateState = ctx->widget.disabled
			? indeterminateTextElem.getState(WidgetStateType::Disabled)
			: indeterminateTextElem.normalState();

		ctx->renderer.cmdSetColor(tintApply(indeterminateState.textColor, TintColorType::Text));
		ctx->renderer.cmdSetFont(bodyElemState->font);
		ctx->renderer.cmdDrawTextInBox(
			indeterminate,
			textRect,
			HAlignType::Left,
			VAlignType::Center, true);
	}

	widgetSetFocusable();

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
		// we need exact width, so don't scale the popup's width
		ctx->popupUseGlobalScale = false;

		popupBegin("popup", ctx->widget.rect.width,
			PopupFlags::CustomPosition,
			popupPos,
			WidgetElementId::DropdownListBody);

		auto& selectableBodyElem = ctx->theme->getElement(WidgetElementId::SelectableBody).normalState();

		spacingPush(0);
		paddingPush(PaddingType::ScrollView, Point());

		if (maxVisibleDropDownItems < itemCount)
		{
			// scroll view doesn't scale height by default, so scale it to match the selectable rows
			f32 scrollViewHeight = std::min(itemCount, maxVisibleDropDownItems) * selectableBodyElem.height * ctx->scale;

			if (ctx->settings.scaleScrollViewHeight)
				scrollViewHeight /= ctx->scale;

			idPush(ctx->id);
			scrollViewBegin("dropDownScrollView", scrollViewHeight, ctx->dropDownScrollViewPos.y, 0.0f, ScrollViewFlags::NoBorder);
		}

		// we don't want tinting for items, just the dropdown is tinted
		tintPush(Color::white);

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

		tintPop();

		if (maxVisibleDropDownItems < itemCount)
		{
			ctx->dropDownScrollViewPos = scrollViewEnd();
			idPop();
		}

		spacingPop();
		paddingPop(PaddingType::ScrollView);

		if (selectedNewItem || popupMustClose())
		{
			popupClose();
			ctx->dropdown.active = false;
		}

		popupEnd();
		ctx->popupUseGlobalScale = true;
	}

	return selectedNewItem;
}

}