#include "horus.h"
#include "types.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "context.h"
#include "util.h"
#include <algorithm>
#include <string.h> // memset

namespace hui
{
bool comboSliderInternal(f32& value, f32 minVal, f32 maxVal, bool useRange, f32 stepsPerPixel, f32 arrowStep)
{
	static bool mouseWasDown = false;
	static bool dragging = false;
	static bool editingText = false;
	static Point dragLastMousePos;
	static char text[64] = "";
	static u32 comboSliderWidgetId = 0;
	static bool requestChangeToOtherComboSlider = false;
	static u32 newComboSliderId = 0;
	auto bodyElem = ctx->theme->getElement(WidgetElementId::ComboSliderBody);
	auto leftArrowElem = ctx->theme->getElement(WidgetElementId::ComboSliderLeftArrow);
	auto rightArrowElem = ctx->theme->getElement(WidgetElementId::ComboSliderRightArrow);
	auto rangeBarElem = ctx->theme->getElement(WidgetElementId::ComboSliderRangeBar);
	bool wasModified = false;

	if (useRange)
	{
		wasModified = clampValue(value, minVal, maxVal);
	}

	// if we're not editing any text of this particular widget id
	if (!editingText || comboSliderWidgetId != ctx->currentWidgetId)
	{
		addWidgetItem(bodyElem.normalState().height * ctx->globalScale);
		buttonBehavior();
	}

	if (dragging && comboSliderWidgetId == ctx->currentWidgetId)
	{
		ctx->widget.pressed = true;
	}

	if (isHovered() || isPressed())
	{
		setMouseCursor(MouseCursorType::Hand);
	}

	bool arrowStepped = false;
	bool arrowHoveredLeft = false;
	bool arrowHoveredRight = false;

	if (isHovered() && ctx->event.mouse.point.x <= ctx->widget.rect.x + leftArrowElem.normalState().image->width)
	{
		arrowHoveredLeft = true;
	}
	else if (isHovered() && ctx->event.mouse.point.x >= ctx->widget.rect.right() - rightArrowElem.normalState().image->width)
	{
		arrowHoveredRight = true;
	}

	if (isClicked()
		&& ctx->event.mouse.point.x <= ctx->widget.rect.x + leftArrowElem.normalState().image->width)
	{
		value -= arrowStep;
		arrowStepped = true;
		if (useRange) wasModified = clampValue(value, minVal, maxVal);
		ctx->widget.changeEnded = true;
	}
	else if (isClicked() && ctx->event.mouse.point.x >= ctx->widget.rect.right() - rightArrowElem.normalState().image->width)
	{
		value += arrowStep;
		arrowStepped = true;
		if (useRange) wasModified = clampValue(value, minVal, maxVal);
		ctx->widget.changeEnded = true;
	}

	if (isClicked() && !dragging && !arrowStepped)
	{
		editingText = true;
		comboSliderWidgetId = ctx->currentWidgetId;
		mouseWasDown = false;
		dragging = false;
		ctx->widget.focusedWidgetId = ctx->currentWidgetId;
		ctx->widget.focused = true;
		ctx->focusChanged = true;
		ctx->textInput.widgetId = ctx->currentWidgetId;
		memset(text, 64, 0);
		toString(value, text, 64);
		ctx->textInput.editNow = true;
		ctx->textInput.selectAllOnFocus = true;
		ctx->textInput.firstMouseDown = true;
		forceRepaint();
		ctx->penPosition.y -= ctx->spacing * ctx->globalScale + bodyElem.normalState().height;
		textInput(text, 64, TextInputValueMode::NumericOnly);
	}
	else
	if (editingText && comboSliderWidgetId == ctx->currentWidgetId)
	{
		textInput(text, 64, TextInputValueMode::NumericOnly);

		if (!ctx->widget.focused
			|| !ctx->textInput.widgetId
			|| requestChangeToOtherComboSlider
			|| (ctx->event.type == InputEvent::Type::Key
				&& ctx->event.key.down
				&& ctx->event.key.code == KeyCode::Esc
				&& ctx->isActiveLayer()))
		{
			editingText = false;
			comboSliderWidgetId = 0;
			value = atof(text);
			if (useRange) wasModified = clampValue(value, minVal, maxVal);
			ctx->widget.changeEnded = true;

			if (requestChangeToOtherComboSlider)
			{
				comboSliderWidgetId = newComboSliderId;
				newComboSliderId = 0;
				requestChangeToOtherComboSlider = false;
			}
		}
	}
	else
	{
		f32 percentFilled = 1.0f - (maxVal - value) / (maxVal - minVal);
		f32 valueWidth = ctx->widget.rect.width;

		if (ctx->event.type == InputEvent::Type::MouseDown
			&& isHovered()
			&& !dragging
			&& ctx->isActiveLayer())
		{
			setCapture(getWindow());
			dragLastMousePos = ctx->event.mouse.point;

			if (!editingText)
			{
				comboSliderWidgetId = ctx->currentWidgetId;
			}
			else
			{
				requestChangeToOtherComboSlider = true;
				newComboSliderId = ctx->currentWidgetId;
				forceRepaint();
			}

			mouseWasDown = true;
		}

		if (mouseWasDown
			&& ctx->event.type == InputEvent::Type::MouseMove
			&& comboSliderWidgetId == ctx->currentWidgetId)
		{
			if (dragLastMousePos.getDistance(ctx->event.mouse.point) > ctx->settings.dragStartDistance)
			{
				dragging = true;
				dragLastMousePos = ctx->event.mouse.point;
				mouseWasDown = false;
			}
		}

		if (dragging
			&& ctx->currentWidgetId == comboSliderWidgetId
			&& ctx->isActiveLayer())
		{
			Point delta = ctx->event.mouse.point - dragLastMousePos;
			dragLastMousePos = ctx->event.mouse.point;
			f32 deltaValue = 0;

			switch (ctx->settings.sliderDragDirection)
			{
			case SliderDragDirection::Any:
				if (fabsf(delta.x) > fabsf(delta.y))
					deltaValue = delta.x;
				else
					deltaValue = ctx->settings.sliderInvertVerticalDragAmount ? delta.y : -delta.y;
				break;
			case SliderDragDirection::VerticalOnly:
				deltaValue = delta.y;
				break;
			case SliderDragDirection::HorizontalOnly:
				deltaValue = delta.x;
				break;
			}

			if (useRange)
			{
				value += deltaValue * stepsPerPixel;
				wasModified = clampValue(value, minVal, maxVal);
				percentFilled = 1.0f - (maxVal - value) / (maxVal - minVal);
			}
			else
			{
				value += deltaValue * stepsPerPixel;
				wasModified = true;
			}
		}

		if (ctx->event.type == InputEvent::Type::MouseUp
			&& (dragging || mouseWasDown)
			&& ctx->isActiveLayer()
			&& comboSliderWidgetId == ctx->currentWidgetId)
		{
			dragging = false;
			mouseWasDown = false;
			comboSliderWidgetId = 0;
			releaseCapture();
			ctx->widget.changeEnded = true;
		}

		auto bodyElemState = &bodyElem.normalState();
		auto leftArrowElemState = &leftArrowElem.normalState();
		auto rightArrowElemState = &rightArrowElem.normalState();
		auto rangeBarElemState = &rangeBarElem.normalState();

		if (ctx->widget.pressed)
		{
			bodyElemState = &bodyElem.getState(WidgetStateType::Pressed);
			leftArrowElemState = &leftArrowElem.getState(WidgetStateType::Pressed);
			rightArrowElemState = &rightArrowElem.getState(WidgetStateType::Pressed);
			rangeBarElemState = &rangeBarElem.getState(WidgetStateType::Pressed);
		}
		else if (ctx->widget.focused)
		{
			bodyElemState = &bodyElem.getState(WidgetStateType::Focused);
			rangeBarElemState = &rangeBarElem.getState(WidgetStateType::Focused);
		}
		else if (ctx->widget.hovered)
		{
			bodyElemState = &bodyElem.getState(WidgetStateType::Hovered);
			rangeBarElemState = &rangeBarElem.getState(WidgetStateType::Hovered);
		}

		if (arrowHoveredLeft)
		{
			leftArrowElemState = &leftArrowElem.getState(WidgetStateType::Hovered);
		}

		if (arrowHoveredRight)
		{
			rightArrowElemState = &rightArrowElem.getState(WidgetStateType::Hovered);
		}

		ctx->renderer->cmdSetColor(bodyElemState->color * ctx->tint[(int)TintColorType::Body]);
		ctx->renderer->cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->globalScale);
		
		if (useRange)
		{
			ctx->renderer->cmdSetColor(rangeBarElemState->color * ctx->tint[(int)TintColorType::Body]);
			ctx->renderer->cmdDrawImageBordered(rangeBarElemState->image, rangeBarElemState->border,
				{
					ctx->widget.rect.x + bodyElemState->border * ctx->globalScale,
					ctx->widget.rect.bottom() - bodyElemState->border * ctx->globalScale,
					(valueWidth - bodyElemState->border * 2.0f * ctx->globalScale) * percentFilled,
					rangeBarElemState->height * ctx->globalScale,
				},
				ctx->globalScale);
		}

		ctx->renderer->cmdSetColor(leftArrowElemState->color * ctx->tint[(int)TintColorType::Body]);

		// dial down the height, since its already global scaled
		auto arrowY = ((ctx->widget.rect.height / ctx->globalScale - leftArrowElemState->image->rect.height) / 2.0f + (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->globalScale;

		ctx->renderer->cmdDrawImage(leftArrowElemState->image,
			{
				ctx->widget.rect.x + bodyElemState->border + (ctx->widget.pressed ? 1.0f : 0.0f) * ctx->globalScale,
				ctx->widget.rect.top() + arrowY,
				leftArrowElemState->image->rect.width * ctx->globalScale,
				leftArrowElemState->image->rect.height * ctx->globalScale
			});

		ctx->renderer->cmdSetColor(rightArrowElemState->color * ctx->tint[(int)TintColorType::Body]);

		// dial down the height, since its already global scaled
		arrowY = ((ctx->widget.rect.height / ctx->globalScale - rightArrowElemState->image->rect.height) / 2.0f + (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->globalScale;

		ctx->renderer->cmdDrawImage(rightArrowElemState->image,
			{
				ctx->widget.rect.right() - bodyElemState->border - (rightArrowElemState->image->rect.width - (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->globalScale,
				ctx->widget.rect.top() + arrowY,
				rightArrowElemState->image->rect.width * ctx->globalScale,
				rightArrowElemState->image->rect.height * ctx->globalScale
			});

		char outStr[64];
		toString(value, outStr, 64);
		ctx->renderer->cmdSetColor(bodyElemState->textColor * ctx->tint[(int)TintColorType::Body]);
		ctx->renderer->cmdDrawTextInBox(outStr, ctx->widget.rect, HAlignType::Center, VAlignType::Center);
		setAsFocusable();
		ctx->currentWidgetId++;
	}

	return wasModified;
}

bool comboSliderFloat(f32& value, f32 stepsPerPixel, f32 arrowStep)
{
	return comboSliderInternal(value, 0, 0, false, stepsPerPixel, arrowStep);
}

bool comboSliderFloatRanged(f32& value, f32 minVal, f32 maxVal, f32 stepsPerPixel, f32 arrowStep)
{
	return comboSliderInternal(value, minVal, maxVal, true, stepsPerPixel, arrowStep);
}

}
