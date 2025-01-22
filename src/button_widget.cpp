#include <algorithm>
#include "context.h"
#include "renderer.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "util.h"

namespace hui
{
void buttonBehavior(bool menuItem)
{
	ctx->widget.hovered = (ctx->id == ctx->widget.hoveredId);
	ctx->widget.focused = (ctx->id == ctx->widget.focusedId);
	ctx->widget.clicked = false;
	ctx->widget.pressed = false;
	ctx->widget.visible = true;
	ctx->dragDrop.allowDrop = false;

	if (!ctx->isActiveLayer() && !menuItem)
		return;

	if (ctx->widget.disabled)
		return;

	if (ctx->id == ctx->widget.focusedId
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
	if (clippedRect.contains(ctx->mousePosition) && ctx->hoveringThisWindow)
	{
		bool alreadyCapturedSomeWidget = ctx->widget.focusedAndPressed && (ctx->id != ctx->widget.focusedId);

		if (!alreadyCapturedSomeWidget)
		{
			ctx->widget.hoveredId = ctx->id;
			ctx->widget.hovered = true;
			ctx->widget.hoveredWidgetRect = ctx->widget.rect;

			if (ctx->event.type == InputEvent::Type::MouseDown
				&& ctx->event.mouse.button == MouseButton::Left)
			{
				ctx->widget.focusedId = ctx->id;
				ctx->widget.pressed = true;
				ctx->widget.focusedAndPressed = true;

				if (ctx->popupIndex)
				{
					auto& popup = ctx->popupStack[ctx->popupIndex - 1];

					popup.alreadyClickedOnSomething = true;
				}

				ctx->alreadyClickedOnSomething = true;
			}
			else if (ctx->event.type == InputEvent::Type::MouseUp
				&& ctx->event.mouse.button == MouseButton::Left)
			{
				if (ctx->id == ctx->widget.focusedId)
				{
					ctx->widget.clicked = true;
					ctx->widget.pressed = false;
					ctx->widget.focusedAndPressed = false;
				}
			}

			ctx->widget.pressed = ctx->widget.focusedAndPressed;
		}
	}
	else
	{
		if (ctx->event.type == InputEvent::Type::MouseDown)
		{
			if (ctx->id == ctx->widget.focusedId)
			{
				ctx->widget.pressed = false;
				ctx->widget.focusedId = 0;
				ctx->widget.focusedAndPressed = false;
			}
		}

		if (ctx->event.type == InputEvent::Type::MouseUp)
		{
			if (ctx->id == ctx->widget.focusedId)
			{
				ctx->widget.pressed = false;
				ctx->widget.clicked = false;
				ctx->widget.focusedAndPressed = false;
			}
		}
	}
}

void mouseDownOnlyButtonBehavior()
{
	ctx->widget.hovered = (ctx->id == ctx->widget.hoveredId);
	ctx->widget.focused = (ctx->id == ctx->widget.focusedId);
	ctx->widget.clicked = false;
	ctx->widget.pressed = false;
	ctx->widget.visible = true;
	ctx->dragDrop.allowDrop = false;

	if (!ctx->isActiveLayer())
		return;

	if (ctx->widget.disabled)
		return;

	// return if the widget is not visible, that is outside current clip rect
	if (ctx->widget.rect.outside(ctx->renderer->getClipRect()))
	{
		ctx->widget.hovered = false;
		ctx->widget.visible = false;
		return;
	}

	Rect clippedRect = ctx->widget.rect.clipInside(ctx->renderer->getClipRect());

	if (clippedRect.contains(ctx->mousePosition) && ctx->hoveringThisWindow)
	{
		ctx->widget.hovered = true;
		ctx->widget.hoveredWidgetRect = ctx->widget.rect;

		if (ctx->widget.hoveredId != ctx->id)
		{
			//ctx->tooltip.timer = 0;
		}

		ctx->widget.hoveredId = ctx->id;

		if (ctx->event.type == InputEvent::Type::MouseDown
			&& ctx->event.mouse.button == MouseButton::Left)
		{
			ctx->widget.focusedId = ctx->id;
			ctx->widget.pressed = true;

			if (ctx->layerIndex)
			{
				auto& popup = ctx->popupStack[ctx->layerIndex - 1];
				popup.alreadyClickedOnSomething = true;
			}

			if (ctx->id == ctx->widget.focusedId)
			{
				ctx->widget.focusedId = 0;
				ctx->widget.pressed = true;
				ctx->widget.clicked = true;
				return;
			}
		}
	}
	else
	{
		if (ctx->event.type == InputEvent::Type::MouseDown
			&& ctx->id == ctx->widget.focusedId)
		{
			ctx->widget.focusedId = 0;
			ctx->widget.focusedAndPressed = false;
			ctx->widget.pressed = false;
		}

		if (ctx->id == ctx->widget.hoveredId)
		{
			ctx->widget.hoveredId = 0;
			ctx->widget.focusedAndPressed = false;
			ctx->widget.pressed = false;
		}
	}
}

bool button(const char* label)
{
	auto& btnBodyElem = ctx->theme->getElement(WidgetElementId::ButtonBody);

	if (ctx->sameLine)
	{
		ctx->extractLabelAndId(label, ctx->widgetLabel, ctx->id);

		auto textWidth = btnBodyElem.normalState().font->computeTextSize(ctx->widgetLabel.c_str());
		ctx->widget.width = (btnBodyElem.normalState().border * 2.0f + textWidth.width) * ctx->scale;
	}

	addWidgetItem(label, btnBodyElem.normalState().height * ctx->scale);

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
		ctx->renderer->cmdSetColor(applyTint(btnBodyElemState->color,TintColorType::Body));
		ctx->renderer->cmdDrawImageBordered(btnBodyElemState->image, btnBodyElemState->border, ctx->widget.rect, ctx->scale);
		ctx->renderer->cmdSetColor(applyTint(btnBodyElemState->textColor,TintColorType::Text));
		ctx->renderer->cmdSetFont(btnBodyElemState->font);
		ctx->renderer->pushClipRect(ctx->widget.rect);
		ctx->renderer->cmdDrawTextInBox(
			ctx->widgetLabel.c_str(),
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

	return ctx->widget.clicked;
}

bool iconButtonInternal(HImage icon, HImage disabledIcon, f32 customHeight, bool down, ThemeElement* btnBodyElem)
{
	auto btnBodyElemState = &btnBodyElem->normalState();
	Image* iconImg = (Image*)icon;
	Image* disabledIconImg = (Image*)disabledIcon;
	f32 height = 0.0f;

	if (!iconImg)
	{
		return false;
	}

	if (customHeight > 0.0f)
		height = customHeight;
	else
		height = std::max(btnBodyElemState->height, iconImg->rect.height);

	addWidgetItem("", height * ctx->scale);
	buttonBehavior();

	f32 pressedIncrement = 0.0f;

	if (ctx->widget.disabled && disabledIconImg)
	{
		btnBodyElemState = &btnBodyElem->getState(WidgetStateType::Disabled);
		iconImg = disabledIconImg;
	}
	else if (ctx->widget.pressed || down || isClicked())
		btnBodyElemState = &btnBodyElem->getState(WidgetStateType::Pressed);
	else if (ctx->widget.focused)
		btnBodyElemState = &btnBodyElem->getState(WidgetStateType::Focused);
	else if (ctx->widget.hovered)
		btnBodyElemState = &btnBodyElem->getState(WidgetStateType::Hovered);

	if (ctx->widget.visible)
	{
		ctx->renderer->cmdSetColor(applyTint(btnBodyElemState->color, TintColorType::Body));
		ctx->renderer->cmdDrawImageBordered(btnBodyElemState->image, btnBodyElemState->border, ctx->widget.rect, ctx->scale);
		ctx->renderer->cmdSetColor(applyTint(btnBodyElemState->textColor, TintColorType::Text));
		ctx->renderer->cmdSetFont(btnBodyElemState->font);
		ctx->renderer->cmdDrawImage(
			iconImg,
			{
				round(ctx->widget.rect.x + (ctx->widget.rect.width - iconImg->rect.width * ctx->scale) / 2 + pressedIncrement * ctx->scale),
				round(ctx->widget.rect.y + (ctx->widget.rect.height - iconImg->rect.height * ctx->scale) / 2 + pressedIncrement * ctx->scale),
				iconImg->rect.width * ctx->scale,
				iconImg->rect.height * ctx->scale
			});
	}

	setFocusable();

	if (isClicked())
		forceRepaint();

	return ctx->widget.clicked;
}

bool iconButton(HImage icon, f32 customHeight, bool down)
{
	auto& btnBodyElem = ctx->theme->getElement(WidgetElementId::ButtonBody);

	return iconButtonInternal(icon, icon, customHeight, down, &btnBodyElem);
}

}