#include <algorithm>
#include "context.h"
#include "renderer.h"
#include "theme.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool button(const char* label)
{
	ctx->setLabelAndId(label);

	auto& btnBodyElem = ctx->theme->getElement(WidgetElementId::ButtonBody);
	auto textWidth = btnBodyElem.normalState().font->computeTextSize(ctx->widgetLabel.c_str());

	ctx->widget.customWidth = btnBodyElem.normalState().border * 2.0f + textWidth.width;
	ctx->widget.hasCustomWidth = true;
	addWidget(btnBodyElem.normalState().height);
	buttonBehavior();

	auto btnBodyElemState = &btnBodyElem.normalState();

	if (ctx->widget.disabled)
		btnBodyElemState = &btnBodyElem.getState(WidgetStateType::Disabled);
	else if (ctx->widget.pressed)
		btnBodyElemState = &btnBodyElem.getState(WidgetStateType::Pressed);
	else if (ctx->widget.focused)
		btnBodyElemState = &btnBodyElem.getState(WidgetStateType::Focused);
	else if (ctx->widget.hovered)
		btnBodyElemState = &btnBodyElem.getState(WidgetStateType::Hovered);

	if (ctx->widget.visible)
	{
		ctx->renderer.cmdSetColor(tintApply(btnBodyElemState->color, TintColorType::Body));
		
		Image* bodyImage = btnBodyElemState->image;
		
		if (!bodyImage && ctx->widget.disabled)
		{
			bodyImage = btnBodyElem.normalState().image;
		}

		ctx->renderer.cmdDrawImageBordered(bodyImage, btnBodyElemState->border, ctx->widget.rect, ctx->scale);

		Rect textRect = ctx->widget.pressed
			? Rect(
				ctx->widget.rect.x + ctx->scale,
				ctx->widget.rect.y + ctx->scale,
				ctx->widget.rect.width,
				ctx->widget.rect.height)
			: ctx->widget.rect;

		// draw text shadow first
		if (btnBodyElemState->textShadow.enabled)
		{
			Rect shadowRect = {
				textRect.x + btnBodyElemState->textShadow.offsetX * ctx->scale,
				textRect.y + btnBodyElemState->textShadow.offsetY * ctx->scale,
				textRect.width,
				textRect.height
			};
			ctx->renderer.cmdSetColor(tintApply(btnBodyElemState->textShadow.color, TintColorType::Text));
			ctx->renderer.cmdSetFont(btnBodyElemState->font);
			ctx->renderer.cmdDrawTextInBox(
				ctx->widgetLabel.c_str(),
				shadowRect,
				HAlignType::Center,
				VAlignType::Center, true);
		}

		// draw main text
		ctx->renderer.cmdSetColor(tintApply(btnBodyElemState->textColor, TintColorType::Text));
		ctx->renderer.cmdSetFont(btnBodyElemState->font);
		ctx->renderer.cmdDrawTextInBox(
			ctx->widgetLabel.c_str(),
			textRect,
			HAlignType::Center,
			VAlignType::Center, true);
	}

	widgetSetFocusable();

	return ctx->widget.clicked;
}

static bool imageButtonInternal(HImage img, HImage disabledImg, f32 width, f32 height, bool down, ThemeElement* btnBodyElem)
{
	auto btnBodyElemState = &btnBodyElem->normalState();
	Image* image = (Image*)img;
	Image* disabledImage = (Image*)disabledImg;

	if (!image)
	{
		return false;
	}

	ctx->widget.customWidth = width;
	ctx->widget.hasCustomWidth = true;

	ctx->setLabelAndId(nullptr);
	addWidget(height);
	buttonBehavior();

	f32 pressedIncrement = 0.0f;

	if (ctx->widget.disabled)
	{
		btnBodyElemState = &btnBodyElem->getState(WidgetStateType::Disabled);

		if (disabledImage)
		{
			image = disabledImage;
		}
	}
	else if (ctx->widget.pressed || down || widgetIsClicked())
	{
		btnBodyElemState = &btnBodyElem->getState(WidgetStateType::Pressed);
		pressedIncrement = 1.0f;
	}
	else if (ctx->widget.focused)
		btnBodyElemState = &btnBodyElem->getState(WidgetStateType::Focused);
	else if (ctx->widget.hovered)
		btnBodyElemState = &btnBodyElem->getState(WidgetStateType::Hovered);

	if (ctx->widget.visible)
	{
		auto imgWidth = image->rect.width * ctx->scale;
		auto imgHeight = image->rect.height * ctx->scale;

		viewportImageSizeFit(imgWidth, imgHeight, ctx->widget.rect.width - (widgetGetPadding().x * 2.0f + btnBodyElemState->border * 2.0f) * ctx->scale, ctx->widget.rect.height - (widgetGetPadding().y * 2.0f + btnBodyElemState->border * 2.0f) * ctx->scale, imgWidth, imgHeight, false, false);

		ctx->renderer.cmdSetColor(tintApply(btnBodyElemState->color, TintColorType::Body));
		
		Image* bodyImage = btnBodyElemState->image;

		if (!bodyImage && ctx->widget.disabled)
		{
			bodyImage = btnBodyElem->normalState().image;
		}

		ctx->renderer.cmdDrawImageBordered(bodyImage, btnBodyElemState->border, ctx->widget.rect, ctx->scale);
		ctx->renderer.cmdSetColor(tintApply(btnBodyElemState->textColor, TintColorType::Text));
		ctx->renderer.cmdSetFont(btnBodyElemState->font);
		ctx->renderer.cmdDrawImage(
			image,
			{
				ctx->widget.rect.x + (ctx->widget.rect.width - imgWidth) / 2 + pressedIncrement * ctx->scale,
				ctx->widget.rect.y + (ctx->widget.rect.height - imgHeight) / 2 + pressedIncrement * ctx->scale,
				imgWidth,
				imgHeight
			});
	}

	widgetSetFocusable();

	if (widgetIsClicked())
		forceRepaint();

	return ctx->widget.clicked;
}

bool imageButton(HImage img, f32 width, f32 height, HImage disabledImg, bool down)
{
	auto& btnBodyElem = ctx->theme->getElement(WidgetElementId::ImageButtonBody);

	return imageButtonInternal(img, disabledImg, width, height, down, &btnBodyElem);
}

void buttonBehavior(bool menuItem)
{
	ctx->widget.hovered = (ctx->id == ctx->widget.hoveredId);
	ctx->widget.focused = (ctx->id == ctx->widget.focusedId);
	ctx->widget.clicked = false;
	ctx->widget.pressed = ctx->widget.captureId == ctx->id;
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
	if (ctx->widget.rect.outside(ctx->renderer.getClipRect()))
	{
		ctx->widget.visible = false;
		return;
	}

	Rect clippedRect = ctx->widget.rect.clipInside(ctx->renderer.getClipRect());

	// if we're inside the button
	if (clippedRect.contains(ctx->mousePosition) && ctx->hoveringThisWindow)
	{
		bool anotherWidgetHasCapture =
			ctx->widget.captureId
			&& ctx->id != ctx->widget.captureId
			&& !ctx->dragDrop.begunDragging;

		if (!anotherWidgetHasCapture)
		{
			ctx->widget.hoveredId = ctx->id;
			ctx->widget.hovered = true;
			ctx->widget.hoveredWidgetRect = ctx->widget.rect;

			if (ctx->event.type == InputEvent::Type::MouseDown
				&& ctx->event.mouse.button == MouseButton::Left)
			{
				ctx->widget.focusedId = ctx->id;
				ctx->widget.captureId = ctx->id;
				ctx->widget.pressed = true;
				ctx->widget.focused = true;

				if (ctx->event.mouse.clickCount == 2)
					ctx->widget.doubleClicked = true;

				if (!ctx->tooltip.ctrlDown)
				{
					ctx->tooltip.show = false;
					ctx->tooltip.lastId = ctx->tooltip.id;
				}

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
				if (ctx->id == ctx->widget.captureId)
				{
					ctx->widget.clicked = true;
					ctx->widget.pressed = false;
					ctx->widget.focused = true;
					ctx->widget.captureId = 0;
				}
			}
		}
	}
	else // outside the widget
	{
		if (ctx->tooltip.lastId == ctx->id)
		{
			ctx->tooltip.lastId = 0;
		}

		if (ctx->event.type == InputEvent::Type::MouseDown)
		{
			if (ctx->id == ctx->widget.focusedId)
			{
				ctx->widget.pressed = false;
				ctx->widget.focused = false;
				ctx->widget.focusedId = 0;
				ctx->widget.captureId = 0;
			}
		}

		if (ctx->event.type == InputEvent::Type::MouseUp)
		{
			if (ctx->id == ctx->widget.focusedId)
			{
				ctx->widget.pressed = false;
				ctx->widget.clicked = false;
				ctx->widget.captureId = 0;
			}
		}
	}
}

void mouseDownOnlyButtonBehavior()
{
	ctx->widget.hovered = (ctx->id == ctx->widget.hoveredId);
	ctx->widget.focused = (ctx->id == ctx->widget.focusedId);
	ctx->widget.clicked = false;
	ctx->widget.pressed = ctx->widget.captureId == ctx->id;
	ctx->widget.visible = true;
	ctx->dragDrop.allowDrop = false;

	if (!ctx->isActiveLayer())
		return;

	if (ctx->widget.disabled)
		return;

	// return if the widget is not visible, that is outside current clip rect
	if (ctx->widget.rect.outside(ctx->renderer.getClipRect()))
	{
		ctx->widget.hovered = false;
		ctx->widget.visible = false;
		return;
	}

	Rect clippedRect = ctx->widget.rect.clipInside(ctx->renderer.getClipRect());

	if (clippedRect.contains(ctx->mousePosition) && ctx->hoveringThisWindow)
	{
		ctx->widget.hovered = true;
		ctx->widget.hoveredWidgetRect = ctx->widget.rect;
		ctx->widget.hoveredId = ctx->id;

		if (ctx->event.type == InputEvent::Type::MouseDown
			&& ctx->event.mouse.button == MouseButton::Left)
		{
			ctx->widget.focusedId = ctx->id;
			ctx->widget.pressed = true;
			ctx->widget.clicked = true;
			ctx->widget.focused = true;

			if (!ctx->tooltip.ctrlDown)
			{
				ctx->tooltip.show = false;
				ctx->tooltip.lastId = ctx->tooltip.id;
			}

			if (ctx->layerIndex)
			{
				auto& popup = ctx->popupStack[ctx->layerIndex - 1];
				popup.alreadyClickedOnSomething = true;
			}
		}
	}
	else // outside widget
	{
		if (ctx->tooltip.lastId == ctx->id)
		{
			ctx->tooltip.lastId = 0;
		}

		if ((ctx->event.type == InputEvent::Type::MouseDown
			|| ctx->event.type == InputEvent::Type::MouseUp)
			&& ctx->id == ctx->widget.focusedId)
		{
			ctx->widget.focusedId = 0;
			ctx->widget.captureId = 0;
			ctx->widget.focused = false;
			ctx->widget.pressed = false;
		}
	}
}

}