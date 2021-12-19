#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "context.h"
#include "util.h"
#include <algorithm>

namespace hui
{
void buttonBehavior(bool menuItem)
{
	ctx->widget.hovered = (ctx->currentWidgetId == ctx->widget.hoveredWidgetId);
	ctx->widget.focused = (ctx->currentWidgetId == ctx->widget.focusedWidgetId);
	ctx->widget.clicked = false;
	ctx->widget.pressed = false;
	ctx->widget.visible = true;
	ctx->dragDropState.allowDrop = false;

	if (!ctx->isActiveLayer() && !menuItem)
		return;

	if (!ctx->widget.enabled)
		return;

	if (ctx->currentWidgetId == ctx->widget.focusedWidgetId
		&& ctx->event.type == InputEvent::Type::Key
		&& (ctx->event.key.code == KeyCode::Enter
			|| ctx->event.key.code == KeyCode::Space))
	{
		if (ctx->event.key.down)
		{
			ctx->widget.pressed = true;
			return;
		}
		else
		{
			if (ctx->widget.pressed)
			{
				ctx->widget.pressed = false;
				ctx->widget.clicked = true;
				return;
			}
		}
	}

	// return if the widget is not visible, that is outside current clip rect
	if (ctx->widget.rect.outside(ctx->renderer->getClipRect()))
	{
		ctx->widget.visible = false;
		return;
	}

	Rect clippedRect = ctx->widget.rect.clipInside(ctx->renderer->getClipRect());

	// if we're inside the button
	if (clippedRect.contains(ctx->event.mouse.point) && ctx->hoveringThisWindow)
	{
		bool alreadyCapturedSomeWidget = ctx->widget.focusedWidgetPressed && (ctx->currentWidgetId != ctx->widget.focusedWidgetId);

		if (!alreadyCapturedSomeWidget)
		{
			ctx->widget.hoveredWidgetId = ctx->currentWidgetId;
			ctx->widget.hovered = true;
			ctx->widget.hoveredWidgetRect = ctx->widget.rect;

			if (ctx->event.type == InputEvent::Type::MouseDown
				&& ctx->event.mouse.button == MouseButton::Left)
			{
				ctx->widget.focusedWidgetId = ctx->currentWidgetId;
				ctx->widget.pressed = true;
				ctx->widget.focusedWidgetPressed = true;

				if (ctx->popupIndex)
				{
					auto& popup = ctx->popupStack[ctx->popupIndex - 1];

					popup.alreadyClickedOnSomething = true;
				}
			}
			else if (ctx->event.type == InputEvent::Type::MouseUp
				&& ctx->event.mouse.button == MouseButton::Left)
			{
				if (ctx->currentWidgetId == ctx->widget.focusedWidgetId)
				{
					ctx->widget.clicked = true;
					ctx->widget.pressed = false;
					ctx->widget.focusedWidgetPressed = false;
				}
			}

			ctx->widget.pressed = ctx->widget.focusedWidgetPressed;
		}
	}
	else
	{
		if (ctx->event.type == InputEvent::Type::MouseDown)
		{
			if (ctx->currentWidgetId == ctx->widget.focusedWidgetId)
			{
				ctx->widget.pressed = false;
				ctx->widget.focusedWidgetId = 0;
				ctx->widget.focusedWidgetPressed = false;
			}
		}

		if (ctx->event.type == InputEvent::Type::MouseUp)
		{
			if (ctx->currentWidgetId == ctx->widget.focusedWidgetId)
			{
				ctx->widget.pressed = false;
				ctx->widget.clicked = false;
				ctx->widget.focusedWidgetPressed = false;
			}
		}
	}
}

void mouseDownOnlyButtonBehavior()
{
	ctx->widget.hovered = (ctx->currentWidgetId == ctx->widget.hoveredWidgetId);
	ctx->widget.focused = (ctx->currentWidgetId == ctx->widget.focusedWidgetId);
	ctx->widget.clicked = false;
	ctx->widget.pressed = false;
	ctx->widget.visible = true;
	ctx->dragDropState.allowDrop = false;

	if (!ctx->isActiveLayer())
		return;

	if (!ctx->widget.enabled)
		return;

	// return if the widget is not visible, that is outside current clip rect
	if (ctx->widget.rect.outside(ctx->renderer->getClipRect()))
	{
		ctx->widget.hovered = false;
		ctx->widget.visible = false;
		return;
	}

	Rect clippedRect = ctx->widget.rect.clipInside(ctx->renderer->getClipRect());

	if (clippedRect.contains(ctx->event.mouse.point) && ctx->hoveringThisWindow
		&& ctx->event.mouse.point.getLength())
	{
		ctx->widget.hovered = true;
		ctx->widget.hoveredWidgetRect = ctx->widget.rect;

		if (ctx->widget.hoveredWidgetId != ctx->currentWidgetId)
		{
			//ctx->tooltip.timer = 0;
		}

		ctx->widget.hoveredWidgetId = ctx->currentWidgetId;

		if (ctx->event.type == InputEvent::Type::MouseDown
			&& ctx->event.mouse.button == MouseButton::Left)
		{
			ctx->widget.focusedWidgetId = ctx->currentWidgetId;
			ctx->widget.pressed = true;

			if (ctx->layerIndex)
			{
				auto& popup = ctx->popupStack[ctx->layerIndex - 1];
				popup.alreadyClickedOnSomething = true;
			}

			if (ctx->currentWidgetId == ctx->widget.focusedWidgetId)
			{
				ctx->widget.focusedWidgetId = 0;
				ctx->widget.pressed = true;
				ctx->widget.clicked = true;
				return;
			}
		}
	}
	else
	{
		if (ctx->event.type == InputEvent::Type::MouseDown
			&& ctx->currentWidgetId == ctx->widget.focusedWidgetId)
		{
			ctx->widget.focusedWidgetId = 0;
			ctx->widget.focusedWidgetPressed = false;
			ctx->widget.pressed = false;
		}

		if (ctx->currentWidgetId == ctx->widget.hoveredWidgetId)
		{
			ctx->widget.hoveredWidgetId = 0;
			ctx->widget.focusedWidgetPressed = false;
			ctx->widget.pressed = false;
		}
	}
}

bool button(const char* labelText)
{
	auto btnBodyElem = ctx->theme->getElement(WidgetElementId::ButtonBody);

	addWidgetItem(btnBodyElem.normalState().height * ctx->globalScale);
	buttonBehavior();

	auto btnBodyElemState = &btnBodyElem.normalState();

	if (ctx->widget.pressed)
		btnBodyElemState = &btnBodyElem.getState(WidgetStateType::Pressed);
	else if (ctx->widget.focused)
		btnBodyElemState = &btnBodyElem.getState(WidgetStateType::Focused);
	else if (ctx->widget.hovered)
		btnBodyElemState = &btnBodyElem.getState(WidgetStateType::Hovered);

	if (ctx->widget.visible)
	{
		ctx->renderer->cmdSetColor(btnBodyElemState->color * ctx->tint[(int)TintColorType::Body]);
		ctx->renderer->cmdDrawImageBordered(btnBodyElemState->image, btnBodyElemState->border, ctx->widget.rect, ctx->globalScale);
		ctx->renderer->cmdSetColor(btnBodyElemState->textColor * ctx->tint[(int)TintColorType::Text]);
		ctx->renderer->cmdSetFont(btnBodyElemState->font);
		ctx->renderer->pushClipRect(ctx->widget.rect);
		ctx->renderer->cmdDrawTextInBox(
			labelText,
			ctx->widget.pressed
			? Rect(
				ctx->widget.rect.x + 1,
				ctx->widget.rect.y + 1,
				ctx->widget.rect.width,
				ctx->widget.rect.height)
			: ctx->widget.rect,
			HAlignType::Center,
			VAlignType::Center);
		ctx->renderer->popClipRect();
	}

	setFocusable();
	ctx->currentWidgetId++;

	return ctx->widget.clicked;
}

bool iconButtonInternal(HImage icon, HImage disabledIcon, f32 customHeight, bool down, UiThemeElement* btnBodyElem, bool focusable)
{
	auto btnBodyElemState = &btnBodyElem->normalState();
	UiImage* iconImg = (UiImage*)icon;
	UiImage* disabledIconImg = (UiImage*)disabledIcon;
	f32 height = 0.0f;

	if (customHeight > 0.0f)
		height = customHeight;
	else
		height = std::max(btnBodyElemState->height, iconImg->rect.height);

	addWidgetItem(height * ctx->globalScale);
	buttonBehavior();

	f32 pressedIncrement = 0.0f;

	if (!ctx->widget.enabled)
	{
		btnBodyElemState = &btnBodyElem->getState(WidgetStateType::Disabled);
		iconImg = disabledIconImg;
	}
	else if (ctx->widget.pressed || down || isClicked())
		btnBodyElemState = &btnBodyElem->getState(WidgetStateType::Pressed);
	else if (ctx->widget.focused && focusable)
		btnBodyElemState = &btnBodyElem->getState(WidgetStateType::Focused);
	else if (ctx->widget.hovered)
		btnBodyElemState = &btnBodyElem->getState(WidgetStateType::Hovered);

	if (ctx->widget.visible)
	{
		ctx->renderer->cmdSetColor(btnBodyElemState->color * ctx->tint[(int)TintColorType::Body]);
		ctx->renderer->cmdDrawImageBordered(btnBodyElemState->image, btnBodyElemState->border, ctx->widget.rect, ctx->globalScale);
		ctx->renderer->cmdSetColor(btnBodyElemState->textColor * ctx->tint[(int)TintColorType::Text]);
		ctx->renderer->cmdSetFont(btnBodyElemState->font);
		ctx->renderer->cmdDrawImage(
			iconImg,
			{ round(ctx->widget.rect.x + (ctx->widget.rect.width - iconImg->rect.width * ctx->globalScale) / 2 + pressedIncrement * ctx->globalScale),
			round(ctx->widget.rect.y + (ctx->widget.rect.height - iconImg->rect.height * ctx->globalScale) / 2 + pressedIncrement * ctx->globalScale),
			iconImg->rect.width * ctx->globalScale, iconImg->rect.height * ctx->globalScale });
	}

	if (focusable)
		setFocusable();
	
	ctx->currentWidgetId++;

	if (isClicked())
		forceRepaint();

	return ctx->widget.clicked;
}

bool iconButton(HImage icon, f32 customHeight, bool down)
{
	auto btnBodyElem = ctx->theme->getElement(WidgetElementId::ButtonBody);

	return iconButtonInternal(icon, icon, customHeight, down, &btnBodyElem);
}

}