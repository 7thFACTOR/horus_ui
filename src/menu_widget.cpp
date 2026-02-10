#include <algorithm>
#include "context.h"
#include "theme.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool beginMenuBar()
{
	auto& menuBarElem = ctx->theme->getElement(WidgetElementId::MenuBarBody);
	f32 height = menuBarElem.normalState().height * ctx->scale;

	ctx->id = genId("__MENUBAR__");

	ctx->layoutStack.push_back(ctx->layout);
	ctx->layout.savedPosition = ctx->position;
	ctx->widget.rect.set(
		round(ctx->position.x),
		round(ctx->position.y),
		ctx->layout.width,
		height);

	//TODO: check if menu bar is visible
	//if (!ctx->widget.visible)
	//	return false;

	ctx->renderer->cmdSetColor(menuBarElem.normalState().color);
	ctx->renderer->cmdDrawImageBordered(menuBarElem.normalState().image, menuBarElem.normalState().border, ctx->widget.rect, ctx->scale);
	ctx->currentMenuBarId = ctx->id;

	return true;
}

void endMenuBar()
{
	auto& menuBarElemState = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
	f32 height = menuBarElemState.height * ctx->scale;

	ctx->position = ctx->layout.savedPosition;
	ctx->layout = ctx->layoutStack.back();
	ctx->layoutStack.pop_back();
	ctx->position.y += height;
	ctx->currentMenuBarId = 0;
}

bool beginMenuInternal(const char* label, SelectableFlags stateFlags, bool contextMenu)
{
	auto& menuBarItemElem = ctx->theme->getElement(WidgetElementId::MenuBarItem);
	auto menuBarItemElemState = menuBarItemElem.normalState();
	
	ctx->setLabelAndId(label);
	
	Utf32String* uniStr = ctx->textCache->getText(ctx->widgetLabel.c_str());
	FontTextSize fsize = menuBarItemElemState.font->computeTextSize(*uniStr);
	auto isMenuBarItem = ctx->menuDepth == 0;

	if (isMenuBarItem || contextMenu)
	{
		ctx->rightSideMenu = true;

		//TODO: externalize this
		const f32 leftIndent = 20;

		f32 width = (leftIndent + std::fmaxf(fsize.width, menuBarItemElemState.width)) * ctx->scale;
		f32 height = menuBarItemElemState.height * ctx->scale;

		ctx->widget.rect.set(
			round(ctx->position.x),
			round(ctx->position.y + menuBarItemElemState.height * ctx->scale - height),
			width,
			height);
		ctx->hoveredSimpleMenuItemMenuDepth = ~0;
		// check if clicked
		mouseDownOnlyButtonBehavior();
		auto thisMenuItemId = ctx->id;

		if (ctx->activeMenuBarItemWidgetId
			&& ctx->activeMenuBarItemWidgetId != thisMenuItemId
			&& ctx->activeMenuBarId == ctx->currentMenuBarId)
		{
			Rect clippedRect = ctx->widget.rect.clipInside(ctx->renderer->getClipRect());

			if (clippedRect.contains(ctx->mousePosition))
			{
				ctx->widget.pressed = true;
				ctx->menuItemChosen = true;
				ctx->switchedToAnotherMainMenu = true;
			}
		}

		if (ctx->widget.pressed
			|| (contextMenu && ctx->contextMenuClicked))
		{
			ctx->activeMenuBarItemWidgetId = ctx->id;
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
					ctx->mousePosition.x,
					ctx->mousePosition.y };
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
			&& ctx->widget.hoveredId == thisMenuItemId)
		{
			menuBarItemElemState = menuBarItemElem.getState(WidgetStateType::Hovered);
		}

		if (!contextMenu)
		{
			ctx->renderer->cmdSetColor(menuBarItemElemState.color);
			ctx->renderer->cmdDrawImageBordered(menuBarItemElemState.image, menuBarItemElemState.border, ctx->widget.rect, ctx->scale);
			ctx->renderer->cmdSetFont(menuBarItemElemState.font);
			ctx->renderer->cmdSetColor(menuBarItemElemState.textColor);
			ctx->renderer->cmdDrawTextInBox(ctx->widgetLabel.c_str(), ctx->widget.rect, HAlignType::Center, VAlignType::Center);
			ctx->position.x += width;
			ctx->position.x = round(ctx->position.x);
		}

		if (ctx->activeMenuBarItemWidgetId == thisMenuItemId
			|| (contextMenu && ctx->contextMenuActive))
		{
			auto& menuBodyElem = ctx->theme->getElement(WidgetElementId::MenuBody);

			ctx->renderer->pushClipRect(ctx->renderer->getWindowRect(), false);
			pushSpacing(0);
			beginPopup("menuPopup",
				ctx->menuStack[ctx->menuDepth].size.x + menuBodyElem.normalState().border * 2.0f + ctx->menuFillerWidth + ctx->menuImageSpace,
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
		//TODO: this needs a proper unique id too
		menuItem(ctx->widgetLabel.c_str(), "", 0, flags);
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
			auto& menuBodyElem = ctx->theme->getElement(WidgetElementId::MenuBody);
			ctx->activeMenuBarItemWidgetWidth = rc.width;
			ctx->renderer->pushClipRect(ctx->renderer->getWindowRect(), false);
			pushSpacing(0);
			beginPopup("menuPopup",
				ctx->menuStack[ctx->menuDepth].size.x + menuBodyElem.normalState().border * 2.0f + ctx->menuFillerWidth + ctx->menuImageSpace,
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

				for (i32 i = 0; i < ctx->maxMenuDepth; i++)
				{
					ctx->menuStack[i].active = false;
				}
			}

			ctx->contextMenuWidgetId = 0;
			ctx->menuDepth = 0;
			closePopup();
			ctx->contextMenuActive = false;
			ctx->pressedOnMenuItem = false;
			ctx->clickedOnASubMenuItem = false;
			ctx->switchedToAnotherMainMenu = false;
			ctx->isSubMenu = false;
		}

		endPopup();
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
				ctx->pressedOnMenuItem = false;
				ctx->clickedOnASubMenuItem = false;
			}

			endPopup();
			popSpacing();
			ctx->renderer->popClipRect();
		}

		ctx->menuDepth--;
	}
}

bool beginMenu(const char* label, SelectableFlags stateFlags)
{
	return beginMenuInternal(label, stateFlags, false);
}

void endMenu()
{
	endMenuInternal(false);
}

bool beginContextMenu(ContextMenuFlags flags)
{
	WidgetId id = ctx->id;
	

	if (ctx->event.type == hui::InputEvent::Type::MouseDown
		&& (ctx->event.mouse.button == MouseButton::Right)
		&& ctx->widget.rect.contains(ctx->mousePosition)
		&& !ctx->activeMenuBarItemWidgetId
		&& !ctx->contextMenuClicked
		&& !ctx->contextMenuWidgetId)
	{
		ctx->contextMenuClicked = true;
		ctx->contextMenuWidgetId = id;
	}

	bool opened = false;

	if ((ctx->contextMenuActive || ctx->contextMenuClicked)
		&& ctx->contextMenuWidgetId == id)
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

bool menuItem(const char* label, const char* shortcut, HImage img, SelectableFlags stateFlags)
{
	auto& menuItemShortcutElem = ctx->theme->getElement(WidgetElementId::MenuItemShortcut);
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::MenuItemBody);
	bool hasImage = img != nullptr;
	bool hasCheck = !!(stateFlags & SelectableFlags::Checkable);
	bool isChecked = !!(stateFlags & SelectableFlags::Checked);

	ctx->setLabelAndId(label);
	addWidget(bodyElem.normalState().height * ctx->scale);
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
	ctx->renderer->cmdSetColor(applyTint(bodyElemState->color, TintColorType::Body));
	ctx->renderer->cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->scale);
	ctx->renderer->cmdSetColor(applyTint(bodyElemState->textColor, TintColorType::Text));

	// render menu item text
	ctx->renderer->cmdSetFont(bodyElemState->font);
	ctx->renderer->pushClipRect(ctx->widget.rect);
	ctx->renderer->cmdDrawTextInBox(
		ctx->widgetLabel.c_str(),
		Rect(
			ctx->widget.rect.x + (ctx->menuItemTextSideSpacing + ctx->menuImageSpace) * ctx->scale,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height)
		,
		HAlignType::Left,
		VAlignType::Center);
	ctx->renderer->popClipRect();

	// render the shortcut text
	if (shortcut)
	{
		ctx->renderer->cmdSetColor(applyTint(shortcutElemState->textColor, TintColorType::Text));
		ctx->renderer->cmdSetFont(shortcutElemState->font);
		ctx->renderer->cmdDrawTextInBox(
			shortcut,
			Rect(
				ctx->widget.rect.x,
				ctx->widget.rect.y,
				ctx->widget.rect.width - (ctx->menuItemTextSideSpacing) * ctx->scale,
				ctx->widget.rect.height)
			,
			HAlignType::Right,
			VAlignType::Center, true);
	}

	if (hasCheck)
	{
		auto& checkMarkElem = ctx->theme->getElement(WidgetElementId::MenuItemCheckMark);
		auto& noCheckMarkElem = ctx->theme->getElement(WidgetElementId::MenuItemNoCheckMark);

		// draw image or check mark
		auto& rc = ctx->widget.rect;

		auto rcImage = Rect(
			rc.x + (ctx->menuImageSpace - noCheckMarkElem.normalState().width) / 2.0f * ctx->scale,
			rc.y + (rc.height - noCheckMarkElem.normalState().image->rect.height * ctx->scale) / 2.0f,
			noCheckMarkElem.normalState().image->rect.width * ctx->scale,
			noCheckMarkElem.normalState().image->rect.height * ctx->scale
		);

		ctx->renderer->cmdSetColor(noCheckMarkElem.normalState().color);
		ctx->renderer->cmdDrawImage(noCheckMarkElem.normalState().image, rcImage);

		if (isChecked)
		{
			ctx->renderer->cmdSetColor(checkMarkElem.normalState().color);
			ctx->renderer->cmdDrawImage(checkMarkElem.normalState().image, rcImage);
		}
	}
	else if (hasImage)
	{
		auto& rc = ctx->widget.rect;
		Image* image = (Image*)img;

		auto rcImage = Rect(
			rc.x + (ctx->menuImageSpace - image->rect.width) / 2.0f * ctx->scale,
			rc.y + (rc.height - image->rect.height * ctx->scale) / 2.0f,
			image->rect.width * ctx->scale,
			image->rect.height * ctx->scale
		);

		ctx->renderer->cmdSetColor(Color::white);
		ctx->renderer->cmdDrawImage(image, rcImage);
	}

	setFocusable();

	auto menuItemTextWidth = bodyElemState->font->computeTextSize(
		*ctx->textCache->getText(ctx->widgetLabel.c_str())).width + menuItemShortcutElem.normalState().font->computeTextSize(*ctx->textCache->getText(shortcut ? shortcut : "")).width;

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
			rc.right() - submenuArrowState->image->rect.width * ctx->scale,
			rc.y + (rc.height - submenuArrowState->image->rect.height * ctx->scale) / 2.0f,
			submenuArrowState->image->rect.width * ctx->scale,
			submenuArrowState->image->rect.height * ctx->scale
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