#include <algorithm>
#include "context.h"
#include "theme.h"
#include "renderer.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool menuBarBegin()
{
	auto& menuBarElem = ctx->theme->getElement(WidgetElementId::MenuBarBody);
	f32 height = menuBarElem.normalState().height * ctx->scale;

	widgetPushDisabled(widgetGetDisabled());

	// the bar starts at the current layout position, the placeholder widget must not advance the cursor
	auto barPos = ctx->position;
	addWidget(0, false);
	ctx->position = barPos;

	ctx->id = genId("__MENUBAR__");

	ctx->layoutStack.push_back(ctx->layout);
	ctx->layout.savedPosition = ctx->position;
	ctx->widget.rect.set(
		ctx->position.x,
		ctx->position.y,
		ctx->layout.width,
		height);

	//TODO: check if menu bar is visible
	//if (!ctx->widget.visible)
	//return false;

	ctx->renderer.cmdSetColor(menuBarElem.normalState().color);
	ctx->renderer.cmdDrawImageBordered(menuBarElem.normalState().image, menuBarElem.normalState().border, ctx->widget.rect, ctx->scale);
	ctx->currentMenuBarId = ctx->id;

	return true;
}

void menuBarEnd()
{
	auto& menuBarElemState = ctx->theme->getElement(WidgetElementId::MenuBarBody).normalState();
	f32 height = menuBarElemState.height * ctx->scale;

	ctx->position = ctx->layout.savedPosition;
	ctx->layout = ctx->layoutStack.back();
	ctx->layoutStack.pop_back();
	ctx->position.y += height;
	ctx->currentMenuBarId = 0;

	widgetPopDisabled();
}

bool beginMenuInternal(const char* label, SelectableFlags stateFlags, bool contextMenu)
{
	auto& menuBarItemElem = ctx->theme->getElement(WidgetElementId::MenuBarItem);
	auto menuBarItemElemState = menuBarItemElem.normalState();
	
	widgetPushDisabled(widgetGetDisabled());

	ctx->setLabelAndId(label);
	
	Utf32String uniStr;
	
	ctx->settings.services.utf8To32(ctx->widgetLabel.c_str(), uniStr);
	FontTextSize fsize = menuBarItemElemState.font->computeTextSize(uniStr);
	auto isMenuBarItem = ctx->menuDepth == 0;

	if (isMenuBarItem || contextMenu)
	{
		ctx->rightSideMenu = true;

		//TODO: externalize this
		const f32 leftIndent = 20;

		f32 width = (leftIndent + std::fmaxf(fsize.width, menuBarItemElemState.width)) * ctx->scale;
		f32 height = menuBarItemElemState.height * ctx->scale;

		if (isMenuBarItem && !contextMenu)
		{
			f32 prevY = ctx->position.y;
			addWidget(menuBarItemElemState.height, false);
			ctx->position.y = prevY;
		}

		ctx->widget.rect.set(
			ctx->position.x,
			ctx->position.y + menuBarItemElemState.height * ctx->scale - height,
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
			Rect clippedRect = ctx->widget.rect.clipInside(ctx->renderer.getClipRect());

			if (clippedRect.contains(ctx->mousePosition))
			{
				ctx->widget.pressed = true;
				ctx->menuItemChosen = true;
				ctx->switchedToAnotherMainMenu = true;
			}
		}

		if ((ctx->widget.pressed && !contextMenu)
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
			skipFrame();
			forceRepaint();
			ctx->menuStack[ctx->menuDepth].size.x = 0;
		}

		if (ctx->widget.disabled)
		{
			menuBarItemElemState = menuBarItemElem.getState(WidgetStateType::Disabled);
		}
		else if (ctx->activeMenuBarItemWidgetId == thisMenuItemId)
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
			ctx->renderer.cmdSetColor(menuBarItemElemState.color);
			ctx->renderer.cmdDrawImageBordered(menuBarItemElemState.image, menuBarItemElemState.border, ctx->widget.rect, ctx->scale);
			ctx->renderer.cmdSetFont(menuBarItemElemState.font);
			ctx->renderer.cmdSetColor(menuBarItemElemState.textColor);
			ctx->renderer.cmdDrawTextInBox(ctx->widgetLabel.c_str(), ctx->widget.rect, HAlignType::Center, VAlignType::Center);
			ctx->position.x += width;

		}

		if (ctx->activeMenuBarItemWidgetId == thisMenuItemId
			|| (contextMenu && ctx->contextMenuActive))
		{
			auto& menuBodyElem = ctx->theme->getElement(WidgetElementId::MenuBody);

			ctx->renderer.pushClipRect(ctx->renderer.getWindowRect(), false);
			spacingPush(0);
			popupBegin("menuPopup",
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
		auto& menuState = ctx->menuStack[ctx->menuDepth];
		bool submenuActive = menuState.active && menuState.id == ctx->id;
		SelectableFlags flags = SelectableFlags::Normal;

		if (submenuActive)
			flags = flags | SelectableFlags::Selected;

		ctx->isSubMenu = true;
		menuItem(ctx->widgetLabel.c_str(), "", 0, flags);
		ctx->isSubMenu = false;
		WidgetId submenuId = ctx->id;

		if (ctx->widget.hovered)
			ctx->hoveredSimpleMenuItemMenuDepth = ~0;

		// a sibling submenu is open from the same menu, close it when this item is hovered
		if (menuState.active && menuState.id != submenuId && ctx->widget.hovered)
		{
			menuState.active = false;
			menuState.id = 0;
		}

		// remember the parent rect so the submenu can close when the mouse leaves
		// both the parent and the open popup
		menuState.lastItemRect = ctx->widget.rect;

		if (!menuState.active && ctx->widget.hovered)
		{
			ctx->menuItemChosen = false;
			ctx->event.type = InputEvent::Type::None;
			menuState.active = true;
			menuState.id = submenuId;
			ctx->setSkipRenderAndInput(true);
			menuState.size.x = 0;
		}

		if (menuState.active && menuState.id == submenuId)
		{
			auto rc = ctx->widget.rect;
			auto& menuBodyElem = ctx->theme->getElement(WidgetElementId::MenuBody);
			ctx->activeMenuBarItemWidgetWidth = rc.width;
			ctx->renderer.pushClipRect(ctx->renderer.getWindowRect(), false);
			spacingPush(0);
			popupBegin("menuPopup",
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
			|| popupPressedEscape()
			|| ((popupClickedOutside()
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
					ctx->menuStack[i].id = 0;
				}
			}

			ctx->contextMenuWidgetId = 0;
			ctx->menuDepth = 0;
			popupClose();
			ctx->contextMenuActive = false;
			ctx->pressedOnMenuItem = false;
			ctx->clickedOnASubMenuItem = false;
			ctx->switchedToAnotherMainMenu = false;
			ctx->isSubMenu = false;
		}

		popupEnd();
		spacingPop();
		ctx->renderer.popClipRect();
		ctx->menuDepth = 0;
		widgetPopDisabled();
	}
	else if (contextMenu && ctx->menuDepth == 1)
	{
		if (ctx->menuItemChosen
			|| popupPressedEscape()
			|| ((popupClickedOutside()
				&& !ctx->pressedOnMenuItem)
				&& !ctx->clickedOnASubMenuItem))
		{
			ctx->contextMenuWidgetId = 0;
			ctx->menuDepth = 0;
			popupClose();
			ctx->contextMenuActive = false;
			ctx->contextMenuClicked = false;
			ctx->pressedOnMenuItem = false;
			ctx->clickedOnASubMenuItem = false;

			popupEnd();
			spacingPop();
			ctx->renderer.popClipRect();
			ctx->menuDepth = 0;
			widgetPopDisabled();
		}
	}
	else if (ctx->menuDepth > 1)
	{
		auto& menu = ctx->menuStack[ctx->menuDepth - 1];

		if (menu.active)
		{
			if (popupClickedOutside()
				&& !ctx->pressedOnMenuItem
				&& !ctx->clickedOnASubMenuItem)
			{
				ctx->menuItemChosen = true;
				ctx->clickedOnASubMenuItem = false;
			}

			auto& popup = ctx->popupStack[ctx->popupIndex - 1];
			Rect popupRect(popup.position.x, popup.position.y, popup.width, popup.height);
			bool mouseOutsideParent = !menu.lastItemRect.contains(ctx->mousePosition);
			bool mouseOutsidePopup = !popupRect.contains(ctx->mousePosition);

			if (ctx->menuItemChosen
				|| popupPressedEscape()
				|| (mouseOutsideParent && mouseOutsidePopup))
			{
				menu.active = false;
				menu.id = 0;
				popupClose();
				ctx->pressedOnMenuItem = false;
				ctx->clickedOnASubMenuItem = false;
			}

			popupEnd();
			spacingPop();
			ctx->renderer.popClipRect();
			widgetPopDisabled();
		}

		ctx->menuDepth--;
	}
}

bool menuBegin(const char* label, SelectableFlags stateFlags)
{
	return beginMenuInternal(label, stateFlags, false);
}

void menuEnd()
{
	endMenuInternal(false);
}

bool contextMenuBegin(ContextMenuFlags flags)
{
	WidgetId id = ctx->id;
	

	if (ctx->isActiveLayer()
		&& ctx->event.type == hui::InputEvent::Type::MouseUp
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

void contextMenuEnd()
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
	addWidget(bodyElem.normalState().height, false);
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

	if (ctx->widget.disabled)
	{
		bodyElemState = &bodyElem.getState(WidgetStateType::Disabled);
		shortcutElemState = &menuItemShortcutElem.getState(WidgetStateType::Disabled);
	}
	else if (ctx->widget.pressed || !!(stateFlags & SelectableFlags::Selected))
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
	ctx->renderer.cmdSetColor(tintApply(bodyElemState->color, TintColorType::Body));
	ctx->renderer.cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->scale);
	ctx->renderer.cmdSetColor(tintApply(bodyElemState->textColor, TintColorType::Text));

	// render menu item text
	ctx->renderer.cmdSetFont(bodyElemState->font);
	ctx->renderer.pushClipRect(ctx->widget.rect);
	ctx->renderer.cmdDrawTextInBox(
		ctx->widgetLabel.c_str(),
		Rect(
			ctx->widget.rect.x + (ctx->menuItemTextSideSpacing + ctx->menuImageSpace) * ctx->scale,
			ctx->widget.rect.y,
			ctx->widget.rect.width,
			ctx->widget.rect.height)
		,
		HAlignType::Left,
		VAlignType::Center);
	ctx->renderer.popClipRect();

	// render the shortcut text
	if (shortcut)
	{
		ctx->renderer.cmdSetColor(tintApply(shortcutElemState->textColor, TintColorType::Text));
		ctx->renderer.cmdSetFont(shortcutElemState->font);
		ctx->renderer.cmdDrawTextInBox(
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

		auto& checkState = ctx->widget.hovered ? checkMarkElem.hoveredState() : checkMarkElem.normalState();
		auto& noCheckState = ctx->widget.hovered ? noCheckMarkElem.hoveredState() : noCheckMarkElem.normalState();
		Image* checkImage = checkState.image ? checkState.image : checkMarkElem.normalState().image;
		Image* noCheckImage = noCheckState.image ? noCheckState.image : noCheckMarkElem.normalState().image;

		auto drawCheckImage = [&](Image* image, const Color& color)
		{
			f32 width = image->rect.width * ctx->scale;
			f32 height = image->rect.height * ctx->scale;
			f32 maxSize = rc.height * 0.6f;

			if (width > maxSize || height > maxSize)
			{
				f32 factor = maxSize / (std::max)(width, height);
				width *= factor;
				height *= factor;
			}

			ctx->renderer.cmdSetColor(color);
			ctx->renderer.cmdDrawImage(image, {
				rc.x + (ctx->menuImageSpace * ctx->scale - width) / 2.0f,
				rc.y + (rc.height - height) / 2.0f,
				width,
				height
			});
		};

		if (noCheckImage && noCheckImage->rect.width > 1.0f && noCheckImage->rect.height > 1.0f)
			drawCheckImage(noCheckImage, noCheckState.color);

		if (isChecked && checkImage)
			drawCheckImage(checkImage, checkState.color);
	}
	else if (hasImage)
	{
		auto& rc = ctx->widget.rect;
		Image* image = (Image*)img;
		float imgWidth = (float)image->rect.width;
		float imgHeight = (float)image->rect.height;

		viewportImageSizeFit(imgWidth, imgHeight, ctx->menuImageSpace * ctx->scale, rc.height, imgWidth, imgHeight, false, false);

		auto rcImage = Rect(
			rc.x + (ctx->menuImageSpace * ctx->scale - imgWidth) / 2.0f,
			rc.y + (rc.height - imgHeight) / 2.0f,
			imgWidth,
			imgHeight
		);

		ctx->renderer.cmdSetColor(Color::white);
		ctx->renderer.cmdDrawImage(image, rcImage);
	}

	widgetSetFocusable();
	
	Utf32String uniStr, uniShortcutStr;

	ctx->settings.services.utf8To32(ctx->widgetLabel.c_str(), uniStr);
	ctx->settings.services.utf8To32(shortcut ? shortcut : "", uniShortcutStr);

	auto menuItemTextWidth = bodyElemState->font->computeTextSize(uniStr).width + menuItemShortcutElem.normalState().font->computeTextSize(uniShortcutStr).width;

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

		if (ctx->widget.hovered || !!(stateFlags & SelectableFlags::Selected))
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

		ctx->renderer.cmdSetColor(submenuArrowState->color);
		ctx->renderer.cmdDrawImage(submenuArrowState->image, rcArrow);
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