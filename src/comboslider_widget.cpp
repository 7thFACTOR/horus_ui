#include <algorithm>
#include <string.h>
#include "context.h"
#include "theme.h"
#include "font.h"
#include "util.h"

namespace hui
{
static bool comboSliderInternal(bool isInt, f32* value, f32 minVal, f32 maxVal, bool useRange, f32 stepsPerPixel, f32 arrowStep, const char* formatStr, u32 decimalPlaces = 4)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::ComboSliderBody);
	auto& leftArrowElem = ctx->theme->getElement(WidgetElementId::ComboSliderLeftArrow);
	auto& rightArrowElem = ctx->theme->getElement(WidgetElementId::ComboSliderRightArrow);
	auto& rangeBarElem = ctx->theme->getElement(WidgetElementId::ComboSliderRangeBar);
	auto& verticalLineElem = ctx->theme->getElement(WidgetElementId::ComboSliderVerticalLine);
	ctx->widget.changeEnded = false;
	bool arrowStepped = false;
	bool arrowHoveredLeft = false;
	bool arrowHoveredRight = false;
	auto& padding = getWidgetPadding();

	if (useRange)
	{
		ctx->widget.changeEnded = clampValue(*value, minVal, maxVal);
	}

	ctx->id = genId((void*)value);
	auto comboId = ctx->id;

	bool notEditingText = (ctx->comboSlider.editingText && ctx->comboSlider.id != ctx->id) || !ctx->comboSlider.editingText;

	if (notEditingText)
	{
		ctx->widget.customWidth = ctx->layout.width/ctx->scale;
		ctx->widget.hasCustomWidth = true;

		addWidget((bodyElem.normalState().height + padding.y * 2.0f) * ctx->scale);
		buttonBehavior();

		if (ctx->comboSlider.dragging && ctx->id == ctx->comboSlider.id)
		{
			ctx->widget.pressed = true;
		}

		auto cursor = 0;

		// check left arrow
		if (isHovered() 
			&& ctx->mousePosition.x <= ctx->widget.rect.x + leftArrowElem.normalState().image->width + padding.x)
		{
			arrowHoveredLeft = true;
		}
		// check right arrow
		else if (isHovered()
			&& ctx->mousePosition.x >= ctx->widget.rect.right() - rightArrowElem.normalState().image->width - padding.x)
		{
			arrowHoveredRight = true;
		}

		if (isClicked()
			&& ctx->mousePosition.x <= ctx->widget.rect.x + leftArrowElem.normalState().image->width + padding.x)
		{
			*value -= arrowStep;
			arrowStepped = true;
			if (useRange) clampValue(*value, minVal, maxVal);
			ctx->widget.changeEnded = true;
		}
		else if (isClicked() && ctx->mousePosition.x >= ctx->widget.rect.right() - rightArrowElem.normalState().image->width - padding.x)
		{
			*value += arrowStep;
			arrowStepped = true;
			if (useRange) clampValue(*value, minVal, maxVal);
			ctx->widget.changeEnded = true;
		}

		if (isHovered() || isPressed())
		{
			if (arrowHoveredLeft || arrowHoveredRight)
				setMouseCursor(MouseCursorType::Arrow);
			else
				setMouseCursor(MouseCursorType::SizeWE);
		}

		if (isClicked() && !ctx->comboSlider.dragging && !arrowStepped)
		{
			ctx->comboSlider.editingText = true;
			ctx->comboSlider.id = ctx->id;
			ctx->comboSlider.mouseWasDown = false;
			ctx->comboSlider.dragging = false;
			ctx->comboSlider.clickedToEditText = true;
			memset(ctx->comboSlider.text, ctx->comboSlider.maxTextSize, 0);
			toStringF32(*value, ctx->comboSlider.text, ComboSliderState::maxTextSize, decimalPlaces);
			
			ctx->textInput.editNow = true;
			ctx->textInput.selectAllOnFocus = true;
			ctx->textInput.firstMouseDown = true;

			ctx->position.y -= ctx->spacing * ctx->scale + bodyElem.normalState().height;

			setNextFocused();
			textInput("comboSliderEditText", ctx->comboSlider.text, ComboSliderState::maxTextSize, TextInputFlags::NumericOnly);
			
			ctx->widget.focusedId = ctx->id;
			ctx->textInput.id = ctx->id;
			ctx->textInput.editNow = true;
			ctx->textInput.selectAllOnFocus = true;
			ctx->textInput.selectionActive = false;
			ctx->textInput.selectingWithMouse = false;
			ctx->textInput.mouseDown = false;
			forceRepaint();
		}
	}
	else
	if (ctx->comboSlider.editingText && ctx->comboSlider.id == comboId)
	{
		bool wasClickedToEdit = ctx->comboSlider.clickedToEditText;

		if (wasClickedToEdit)
		{
			setNextFocused();
			ctx->comboSlider.clickedToEditText = false;
		}

		textInput("comboSliderEditText", ctx->comboSlider.text, ComboSliderState::maxTextSize, TextInputFlags::NumericOnly);

		if (wasClickedToEdit)
		{
			ctx->textInput.selectingWithMouse = false;
			ctx->textInput.mouseDownSelectionBegin = false;
			ctx->textInput.selectionActive = true;
			ctx->textInput.selectAll();
			ctx->textInput.mouseDown = false;
		}

		bool isKeyEvent = ctx->event.key.down && ctx->event.type == InputEvent::Type::Key;
		bool isEscPressed = isKeyEvent && ctx->event.key.code == KeyCode::Esc;
		bool isEnterPressed = isKeyEvent && ctx->event.key.code == KeyCode::Enter;

		if ((!ctx->textInput.id
			|| ctx->comboSlider.requestChangeToOtherComboSlider)
			&& (isEnterPressed || isEscPressed || ctx->widget.focusedId != ctx->id)
			&& ctx->isActiveLayer())
		{
			ctx->comboSlider.editingText = false;
			ctx->comboSlider.id = 0;
			ctx->widget.changeEnded = true;
			releaseWindowCapture();

			if (!isEscPressed)
			{
				*value = atof(ctx->comboSlider.text);
				if (useRange) clampValue(*value, minVal, maxVal);
			}

			if (ctx->comboSlider.requestChangeToOtherComboSlider)
			{
				ctx->comboSlider.id = ctx->comboSlider.newId;
				ctx->comboSlider.newId = 0;
				ctx->comboSlider.requestChangeToOtherComboSlider = false;
			}
		}
	}
		
	if (notEditingText)
	{
		f32 percentFilled = 1.0f - (maxVal - *value) / (maxVal - minVal);
		f32 valueWidth = ctx->widget.rect.width;

		if (ctx->event.type == InputEvent::Type::MouseDown
			&& isHovered()
			&& !ctx->comboSlider.dragging
			&& ctx->isActiveLayer())
		{
			ctx->comboSlider.dragLastMousePos = ctx->mousePosition;

			if (!ctx->comboSlider.editingText)
			{
				ctx->comboSlider.id = ctx->id;
			}
			else
			{
				ctx->comboSlider.requestChangeToOtherComboSlider = true;
				ctx->comboSlider.newId = ctx->id;
				forceRepaint();
			}

			ctx->comboSlider.mouseWasDown = true;
			setWindowCapture();
		}

		if (ctx->comboSlider.mouseWasDown
			&& ctx->event.type == InputEvent::Type::MouseMove
			&& ctx->comboSlider.id == ctx->id)
		{
			if (ctx->comboSlider.dragLastMousePos.getDistance(ctx->mousePosition) > ctx->settings.dragStartDistance)
			{
				ctx->comboSlider.dragging = true;
				ctx->comboSlider.dragLastMousePos = ctx->mousePosition;
				ctx->comboSlider.mouseWasDown = false;
				ctx->comboSlider.currentValue = *value;
			}
		}

		if (ctx->comboSlider.dragging
			&& ctx->id == ctx->comboSlider.id
			&& ctx->isActiveLayer())
		{
			Point delta = ctx->mousePosition - ctx->comboSlider.dragLastMousePos;
			ctx->comboSlider.dragLastMousePos = ctx->mousePosition;
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
				ctx->comboSlider.currentValue += deltaValue * stepsPerPixel;
				ctx->widget.changeEnded = clampValue(ctx->comboSlider.currentValue, minVal, maxVal);
				percentFilled = 1.0f - (maxVal - ctx->comboSlider.currentValue) / (maxVal - minVal);
				*value = ctx->comboSlider.currentValue;
			}
			else
			{
				ctx->comboSlider.currentValue += deltaValue * stepsPerPixel;
				ctx->widget.changeEnded = true;
				*value = ctx->comboSlider.currentValue;
			}
		}

		if (ctx->event.type == InputEvent::Type::MouseUp
			&& (ctx->comboSlider.dragging || ctx->comboSlider.mouseWasDown)
			&& ctx->isActiveLayer()
			&& ctx->comboSlider.id == ctx->id)
		{
			ctx->comboSlider.dragging = false;
			ctx->comboSlider.mouseWasDown = false;
			ctx->comboSlider.id = 0;
			*value = ctx->comboSlider.currentValue;
			if (isInt) *value = roundf(*value);
			releaseWindowCapture();
			ctx->widget.changeEnded = true;
		}

		auto bodyElemState = &bodyElem.normalState();
		auto leftArrowElemState = &leftArrowElem.normalState();
		auto rightArrowElemState = &rightArrowElem.normalState();
		auto rangeBarElemState = &rangeBarElem.normalState();
		auto verticalLineElemState = &verticalLineElem.normalState();

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

		ctx->renderer.cmdSetColor(applyTint(bodyElemState->color, TintColorType::Body));
		ctx->renderer.cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->scale);
		
		if (useRange)
		{
			ctx->renderer.cmdSetColor(applyTint(rangeBarElemState->color, TintColorType::Body));
			ctx->renderer.cmdDrawImageBordered(rangeBarElemState->image, rangeBarElemState->border,
				{
					ctx->widget.rect.x + bodyElemState->border * ctx->scale,
					ctx->widget.rect.bottom() - bodyElemState->border * ctx->scale,
					(valueWidth - bodyElemState->border * 2.0f * ctx->scale) * percentFilled,
					rangeBarElemState->height * ctx->scale,
				},
				ctx->scale);
		}


		auto arrowY = ((ctx->widget.rect.height - leftArrowElemState->image->height * ctx->scale) / 2.0f + (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->scale;

		ctx->renderer.cmdSetColor(applyTint(leftArrowElemState->color, TintColorType::Body));
		ctx->renderer.cmdDrawImage(leftArrowElemState->image,
			{
				ctx->widget.rect.x + (bodyElemState->border + padding.x + (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->scale,
				ctx->widget.rect.top() + arrowY,
				leftArrowElemState->image->width * ctx->scale,
				leftArrowElemState->image->height * ctx->scale
			});


		auto lineHeight = verticalLineElemState->height + padding.y * 2.0f;
		auto lineY = ((ctx->widget.rect.height - lineHeight * ctx->scale) / 2.0f + (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->scale;

		ctx->renderer.cmdSetColor(applyTint(verticalLineElemState->color, TintColorType::Body));

		ctx->renderer.cmdDrawImage(verticalLineElemState->image,
			{
				ctx->widget.rect.x + (bodyElemState->border + padding.x * 2.0f + leftArrowElemState->image->width + (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->scale,
				ctx->widget.rect.top() + lineY,
				verticalLineElemState->image->width * ctx->scale,
				lineHeight
			});

		arrowY = ((ctx->widget.rect.height - rightArrowElemState->image->rect.height * ctx->scale) / 2.0f + (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->scale;

		ctx->renderer.cmdSetColor(applyTint(rightArrowElemState->color, TintColorType::Body));
		ctx->renderer.cmdDrawImage(rightArrowElemState->image,
			{
				ctx->widget.rect.right() - (bodyElemState->border + padding.x + rightArrowElemState->image->width + (ctx->widget.pressed ? -1.0f : 0.0f)) * ctx->scale,
				ctx->widget.rect.top() + arrowY,
				rightArrowElemState->image->width * ctx->scale,
				rightArrowElemState->image->height * ctx->scale
			});

		ctx->renderer.cmdSetColor(applyTint(verticalLineElemState->color, TintColorType::Body));

		ctx->renderer.cmdDrawImage(verticalLineElemState->image,
			{
				ctx->widget.rect.right() - (bodyElemState->border + padding.x * 2.0f + rightArrowElemState->image->width + (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->scale,
				ctx->widget.rect.top() + lineY,
				verticalLineElemState->image->width * ctx->scale,
				lineHeight
			});

		static char outStr[ComboSliderState::maxTextSize] = { 0 };
		static char outStrFormatted[ComboSliderState::maxTextSize] = { 0 };
		char* str = nullptr;

		toStringF32(*value, outStr, ComboSliderState::maxTextSize, decimalPlaces);

		if (!formatStr)
		{
			str = outStr;
		}
		else
		{
			sprintf(outStrFormatted, formatStr, *value);
			str = outStrFormatted;
		}

		ctx->renderer.cmdSetColor(applyTint(bodyElemState->textColor, TintColorType::Body));
		ctx->renderer.cmdDrawTextInBox(str, ctx->widget.rect, HAlignType::Center, VAlignType::Center);
		setFocusable();
	}

	return ctx->widget.changeEnded;
}

bool comboSliderInteger(i32* value, f32 stepsPerPixel, i32 arrowStep, const char* formatStr)
{
	f32 fVal = *value;

	pushId((void*)value);
	bool ret = comboSliderInternal(true, &fVal, 0, 0, false, stepsPerPixel, (f32)arrowStep, formatStr, 0);
	popId();

	if (ret)
		*value = (i32)fVal;

	return ret;
}

bool comboSliderIntegerRanged(i32* value, i32 minVal, i32 maxVal, f32 stepsPerPixel, i32 arrowStep, const char* formatStr)
{
	f32 fVal = *value;
	pushId((void*)value);
	bool ret = comboSliderInternal(true, &fVal, (f32)minVal, (f32)maxVal, true, stepsPerPixel, (f32)arrowStep, formatStr, 0);
	popId();

	if (ret)
		*value = (i32)fVal;
	
	return ret;
}

bool comboSliderFloat(f32* value, f32 stepsPerPixel, f32 arrowStep, const char* formatStr)
{
	return comboSliderInternal(false, value, 0, 0, false, stepsPerPixel, arrowStep, formatStr);
}

bool comboSliderFloatRanged(f32* value, f32 minVal, f32 maxVal, f32 stepsPerPixel, f32 arrowStep, const char* formatStr)
{
	return comboSliderInternal(false, value, minVal, maxVal, true, stepsPerPixel, arrowStep, formatStr);
}

}
