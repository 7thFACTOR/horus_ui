#include "horus.h"
#include "types.h"
#include "theme.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "context.h"
#include "util.h"
#include <algorithm>

namespace hui
{
void beginMenuBar()
{
	auto menuBarElem = ctx->theme->getElement(WidgetElementId::MenuBarBody);
	f32 height = menuBarElem.normalState().height * ctx->globalScale;

	ctx->widget.rect.set(
		round(ctx->penPosition.x),
		round(ctx->penPosition.y),
		ctx->layoutStack.back().width,
		height);
	ctx->renderer->cmdSetColor(menuBarElem.normalState().color);
	ctx->renderer->cmdDrawImageBordered(menuBarElem.normalState().image, menuBarElem.normalState().border, ctx->widget.rect, ctx->globalScale);
	ctx->currentMenuBarId = ctx->currentWidgetId;
	ctx->currentWidgetId++;
}

void endMenuBar()
{
	auto menuBarElemState = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
	f32 height = menuBarElemState.height * ctx->globalScale;

	ctx->penPosition.x = ctx->layoutStack.back().position.x;
	ctx->penPosition.y += height;
	ctx->currentMenuBarId = 0;
}

bool beginMenuInternal(const char* labelText, SelectableFlags stateFlags, bool contextMenu)
{
	auto menuBarItemElem = ctx->theme->getElement(WidgetElementId::MenuBarItem);
	auto menuBarItemElemState = menuBarItemElem.normalState();
	UnicodeString* uniStr = ctx->textCache->getText(labelText);
	FontTextSize fsize = menuBarItemElemState.font->computeTextSize(*uniStr);
	auto isMenuBarItem = ctx->menuDepth == 0;

	if (isMenuBarItem || contextMenu)
	{
		ctx->rightSideMenu = true;

		//TODO: externalize this
		const f32 leftIndent = 20;

		f32 width = (leftIndent + std::fmaxf(fsize.width, menuBarItemElemState.width)) * ctx->globalScale;
		f32 height = menuBarItemElemState.height * ctx->globalScale;

		ctx->widget.rect.set(
			round(ctx->penPosition.x),
			round(ctx->penPosition.y + menuBarItemElemState.height * ctx->globalScale - height),
			width,
			height);
		ctx->hoveredSimpleMenuItemMenuDepth = ~0;
		// check if clicked
		mouseDownOnlyButtonBehavior();
		auto thisMenuItemId = ctx->currentWidgetId;

		if (ctx->activeMenuBarItemWidgetId
			&& ctx->activeMenuBarItemWidgetId != thisMenuItemId
			&& ctx->activeMenuBarId == ctx->currentMenuBarId)
		{
			Rect clippedRect = ctx->widget.rect.clipInside(ctx->renderer->getClipRect());

			if (clippedRect.contains(ctx->event.mouse.point))
			{
				ctx->widget.pressed = true;
				ctx->menuItemChosen = true;
				ctx->switchedToAnotherMainMenu = true;
			}
		}

		if (ctx->widget.pressed
			|| (contextMenu && ctx->contextMenuClicked))
		{
			ctx->activeMenuBarItemWidgetId = ctx->currentWidgetId;
			ctx->contextMenuClicked = false;

			if (!contextMenu)
			{
				ctx->activeMenuBarItemWidgetPos = {
				ctx->widget.rect.x,
				ctx->widget.rect.bottom() };
				ctx->activeMenuBarItemWidgetWidth = ctx->widget.rect.width;
			}
			else
			{
				ctx->contextMenuActive = true;
				ctx->activeMenuBarItemWidgetPos = {
					ctx->event.mouse.point.x,
					ctx->event.mouse.point.y };
			}

			ctx->event.type = InputEvent::Type::None;
			ctx->menuDepth = 0;
			ctx->activeMenuBarId = ctx->currentMenuBarId;
			skipThisFrame();
			forceRepaint();
			ctx->menuStack[ctx->menuDepth].size.x = 0;
		}

		if (ctx->activeMenuBarItemWidgetId == thisMenuItemId)
		{
			menuBarItemElemState = menuBarItemElem.getState(WidgetStateType::Pressed);
		}
		else if (ctx->widget.hovered
			&& ctx->widget.hoveredWidgetId == thisMenuItemId)
		{
			menuBarItemElemState = menuBarItemElem.getState(WidgetStateType::Hovered);
		}

		if (!contextMenu)
		{
			ctx->renderer->cmdSetColor(menuBarItemElemState.color);
			ctx->renderer->cmdDrawImageBordered(menuBarItemElemState.image, menuBarItemElemState.border, ctx->widget.rect, ctx->globalScale);
			ctx->renderer->cmdSetFont(menuBarItemElemState.font);
			ctx->renderer->cmdSetColor(menuBarItemElemState.textColor);
			ctx->renderer->cmdDrawTextInBox(labelText, ctx->widget.rect, HAlignType::Center, VAlignType::Center);
			ctx->penPosition.x += width;
			ctx->penPosition.x = round(ctx->penPosition.x);
		}

		ctx->currentWidgetId++;

		if (ctx->activeMenuBarItemWidgetId == thisMenuItemId
			|| (contextMenu && ctx->contextMenuActive))
		{
			auto menuBodyElem = ctx->theme->getElement(WidgetElementId::MenuBody);

			ctx->renderer->pushClipRect(ctx->renderer->getWindowRect(), false);
			pushPadding(0);
			pushSpacing(0);
			beginPopup(
				ctx->menuStack[ctx->menuDepth].size.x + menuBodyElem.normalState().border * 2.0f + ctx->menuFillerWidth + ctx->menuIconSpace,
				(contextMenu ? PopupFlags::CustomPosition : PopupFlags::BelowLastWidget) 
				 | PopupFlags::IsMenu,
				ctx->activeMenuBarItemWidgetPos, WidgetElementId::MenuBody);

			ctx->menuDepth++;
		}

		return ctx->activeMenuBarItemWidgetId == thisMenuItemId;
	}
	else
	{
		SelectableFlags flags = SelectableFlags::Normal;

		if (ctx->menuStack[ctx->menuDepth].active)
			flags = flags | SelectableFlags::Selected;

		ctx->isSubMenu = true;
		menuItem(labelText, "", 0, flags);
		ctx->isSubMenu = false;

		if (ctx->widget.hovered)
			ctx->hoveredSimpleMenuItemMenuDepth = ~0;

		if (!ctx->menuStack[ctx->menuDepth].active && (ctx->widget.hovered))
		{
			ctx->menuItemChosen = false;
			ctx->event.type = InputEvent::Type::None;
			ctx->menuStack[ctx->menuDepth].active = true;
			ctx->setSkipRenderAndInput(true);
			ctx->menuStack[ctx->menuDepth].size.x = 0;
		}

		if (ctx->menuStack[ctx->menuDepth].active)
		{
			auto rc = getWidgetRect();

			auto menuBodyElem = ctx->theme->getElement(WidgetElementId::MenuBody);
			ctx->activeMenuBarItemWidgetWidth = rc.width;
			ctx->renderer->pushClipRect(ctx->renderer->getWindowRect(), false);
			pushPadding(0);
			pushSpacing(0);
			beginPopup(
				ctx->menuStack[ctx->menuDepth].size.x + menuBodyElem.normalState().border * 2.0f + ctx->menuFillerWidth + ctx->menuIconSpace,
				PopupFlags::CustomPosition | PopupFlags::IsMenu | PopupFlags::SameLayer,
				Point(rc.right(), rc.top()),
				WidgetElementId::MenuBody);
			ctx->menuDepth++;

			return true;
		}
	}

	return false;
}

void endMenuInternal(bool contextMenu)
{
	if (ctx->activeMenuBarItemWidgetId && ctx->menuDepth == 1)
	{
		if (ctx->menuItemChosen
			|| pressedEscapeOnPopup()
			|| ((clickedOutsidePopup()
				&& !ctx->pressedOnMenuItem)
				&& !ctx->clickedOnASubMenuItem))
		{
			if (!ctx->switchedToAnotherMainMenu)
			{
				ctx->activeMenuBarItemWidgetId = 0;
				ctx->activeMenuBarId = 0;

				for (int i = 0; i < ctx->maxMenuDepth; i++)
				{
					ctx->menuStack[i].active = false;
				}
			}

			ctx->contextMenuWidgetId = 0;
			ctx->menuDepth = 0;
			closePopup();
			ctx->contextMenuActive = false;
			ctx->widget.focusedWidgetPressed = false;
			ctx->pressedOnMenuItem = false;
			ctx->clickedOnASubMenuItem = false;
			ctx->switchedToAnotherMainMenu = false;
			ctx->isSubMenu = false;
		}

		endPopup();
		popPadding();
		popSpacing();
		ctx->renderer->popClipRect();
		ctx->menuDepth = 0;
	}
	else if (ctx->menuDepth > 1)
	{
		auto& menu = ctx->menuStack[ctx->menuDepth - 1];

		if (menu.active)
		{
			if (clickedOutsidePopup()
				&& !ctx->pressedOnMenuItem
				&& !ctx->clickedOnASubMenuItem)
			{
				ctx->menuItemChosen = true;
				ctx->clickedOnASubMenuItem = false;
			}

			if (ctx->menuItemChosen
				|| pressedEscapeOnPopup()
				|| ctx->hoveredSimpleMenuItemMenuDepth < ctx->menuDepth - 1)
			{
				menu.active = false;
				closePopup();
				ctx->widget.focusedWidgetPressed = false;
				ctx->pressedOnMenuItem = false;
				ctx->clickedOnASubMenuItem = false;
			}

			endPopup();
			popPadding();
			popSpacing();
			ctx->renderer->popClipRect();
		}

		ctx->menuDepth--;
	}
}

bool beginMenu(const char* labelText, SelectableFlags stateFlags)
{
	return beginMenuInternal(labelText, stateFlags, false);
}

void endMenu()
{
	endMenuInternal(false);
}

bool beginContextMenu(ContextMenuFlags flags)
{
	u32 widgetId = ctx->currentWidgetId;
	bool leftButton = has(flags, ContextMenuFlags::AllowLeftClickOpen) ? ctx->event.mouse.button == MouseButton::Left : false;

	if (ctx->event.type == hui::InputEvent::Type::MouseDown
		&& (ctx->event.mouse.button == MouseButton::Right || leftButton) 
		&& ctx->widget.rect.contains(ctx->event.mouse.point)
		&& !ctx->activeMenuBarItemWidgetId
		&& !ctx->contextMenuClicked
		&& !ctx->contextMenuWidgetId)
	{
		ctx->contextMenuClicked = true;
		ctx->contextMenuWidgetId = widgetId;
		ctx->widget.focusedWidgetPressed = false;
	}

	bool opened = false;

	if ((ctx->contextMenuActive || ctx->contextMenuClicked)
		&& ctx->contextMenuWidgetId == widgetId)
	{
		opened = true;
		beginMenuInternal("", SelectableFlags::Selected, true);
	}

	return opened;
}

void endContextMenu()
{
	endMenuInternal(true);
}

bool menuItem(const char* labelText, const char* shortcut, HImage icon, SelectableFlags stateFlags)
{
	auto menuItemShortcutElem = ctx->theme->getElement(WidgetElementId::MenuItemShortcut);
	auto bodyElem = ctx->theme->getElement(WidgetElementId::MenuItemBody);
	bool hasIcon = icon != nullptr;
	bool hasCheck = !!(stateFlags & SelectableFlags::Checkable);
	bool isChecked = !!(stateFlags & SelectableFlags::Checked);

	addWidgetItem(bodyElem.normalState().height * ctx->globalScale);
	buttonBehavior(true);

	if (
		!ctx->isActiveLayer()
		&& ctx->widget.hovered
		&& ctx->hoveredSimpleMenuItemMenuDepth == ~0)
	{
		ctx->widget.hovered = false;
	}

	auto bodyElemState = &bodyElem.normalState();
	auto shortcutElemState = &menuItemShortcutElem.normalState();

	if (ctx->widget.pressed || !!(stateFlags & SelectableFlags::Selected))
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);
		shortcutElemState = &menuItemShortcutElem.getState(WidgetStateType::Pressed);
	}
	else if (ctx->widget.focused)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Focused);
		shortcutElemState = &menuItemShortcutElem.getState(WidgetStateType::Focused);
	}
	else if (ctx->widget.hovered)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Hovered);
		shortcutElemState = &menuItemShortcutElem.getState(WidgetStateType::Hovered);
	}

	// render menu item bg
	ctx->renderer->cmdSetColor(bodyElemState->color * ctx->tint[(u32)TintColorType::Body]);
	ctx->renderer->cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->globalScale);
	ctx->renderer->cmdSetColor(bodyElemState->textColor * ctx->tint[(u32)TintColorType::Text]);

	// render menu item text
	ctx->renderer->cmdSetFont(bodyElemState->font);
	ctx->renderer->pushClipRect(ctx->widget.rect);
	ctx->renderer->cmdDrawTextInBox(
		labelText,
		Rect(
			ctx->widget.rect.x + (ctx->menuItemTextSideSpacing + ctx->menuIconSpace) * ctx->globalScale,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height)
		,
		HAlignType::Left,
		VAlignType::Center);
	ctx->renderer->popClipRect();

	// render the shortcut text
	ctx->renderer->cmdSetColor(shortcutElemState->textColor * ctx->tint[(u32)TintColorType::Text]);
	ctx->renderer->cmdSetFont(shortcutElemState->font);
	ctx->renderer->pushClipRect(ctx->widget.rect);
	ctx->renderer->cmdDrawTextInBox(
		shortcut,
		Rect(
			ctx->widget.rect.x,
			ctx->widget.rect.y,
			ctx->widget.rect.width - (ctx->menuItemTextSideSpacing) * ctx->globalScale,
			ctx->widget.rect.height)
		,
		HAlignType::Right,
		VAlignType::Center);
	ctx->renderer->popClipRect();

	if (hasCheck)
	{
		auto checkMarkElem = ctx->theme->getElement(WidgetElementId::MenuItemCheckMark);
		auto noCheckMarkElem = ctx->theme->getElement(WidgetElementId::MenuItemNoCheckMark);

		// draw icon or check mark
		auto& rc = ctx->widget.rect;

		auto rcIcon = Rect(
			rc.x + (ctx->menuIconSpace - noCheckMarkElem.normalState().width) / 2.0f * ctx->globalScale,
			rc.y + (rc.height - noCheckMarkElem.normalState().image->rect.height * ctx->globalScale) / 2.0f,
			noCheckMarkElem.normalState().image->rect.width * ctx->globalScale,
			noCheckMarkElem.normalState().image->rect.height * ctx->globalScale
		);

		ctx->renderer->cmdSetColor(noCheckMarkElem.normalState().color);
		ctx->renderer->cmdDrawImage(noCheckMarkElem.normalState().image, rcIcon);

		if (isChecked)
		{
			ctx->renderer->cmdSetColor(checkMarkElem.normalState().color);
			ctx->renderer->cmdDrawImage(checkMarkElem.normalState().image, rcIcon);
		}
	}
	else if (hasIcon)
	{
		// draw icon
		auto& rc = ctx->widget.rect;
		UiImage* iconImg = (UiImage*)icon;

		auto rcIcon = Rect(
			rc.x + (ctx->menuIconSpace - iconImg->rect.width) / 2.0f * ctx->globalScale,
			rc.y + (rc.height - iconImg->rect.height * ctx->globalScale) / 2.0f,
			iconImg->rect.width * ctx->globalScale,
			iconImg->rect.height * ctx->globalScale
		);

		ctx->renderer->cmdSetColor(Color::white);
		ctx->renderer->cmdDrawImage(iconImg, rcIcon);
	}

	setAsFocusable();
	ctx->currentWidgetId++;
	auto menuItemTextWidth = bodyElemState->font->computeTextSize(
		*ctx->textCache->getText(labelText)).width + menuItemShortcutElem.normalState().font->computeTextSize(*ctx->textCache->getText(shortcut)).width;

	ctx->menuStack[ctx->menuDepth - 1].size.x = std::max(
		menuItemTextWidth,
		ctx->menuStack[ctx->menuDepth - 1].size.x);

	if (!ctx->isSubMenu)
	{
		if (ctx->widget.clicked)
		{
			ctx->menuItemChosen = true;
		}

		if (ctx->widget.pressed)
		{
			ctx->pressedOnMenuItem = true;
		}

		if (ctx->widget.hovered)
			ctx->hoveredSimpleMenuItemMenuDepth = ctx->menuDepth - 1;
	}
	else
	{
		if (ctx->widget.clicked || ctx->widget.pressed)
			ctx->clickedOnASubMenuItem = true;

		ctx->widget.clicked = false;
		ctx->widget.pressed = false;

		// draw arrow
		auto submenuArrowState = &ctx->theme->getElement(WidgetElementId::SubMenuItemArrow).normalState();

		if (ctx->widget.hovered)
		{
			submenuArrowState = &ctx->theme->getElement(WidgetElementId::SubMenuItemArrow).getState(WidgetStateType::Hovered);
		}

		auto& rc = ctx->widget.rect;

		auto rcArrow = Rect(
			rc.right() - submenuArrowState->image->rect.width * ctx->globalScale,
			rc.y + (rc.height - submenuArrowState->image->rect.height * ctx->globalScale) / 2.0f,
			submenuArrowState->image->rect.width * ctx->globalScale,
			submenuArrowState->image->rect.height * ctx->globalScale
		);

		ctx->renderer->cmdSetColor(submenuArrowState->color);
		ctx->renderer->cmdDrawImage(submenuArrowState->image, rcArrow);
	}

	return ctx->widget.clicked;
}

void closeMenu()
{
	ctx->menuItemChosen = true;
}

void menuSeparator()
{
	line();
}

}