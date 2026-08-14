#include <algorithm>
#include <string.h>
#include "context.h"
#include "theme.h"
#include "font.h"
#include "util.h"

namespace hui
{
static void comboSliderElementBehavior(WidgetId elementId, const Rect& rect, ComboSliderElementState& state)
{
	state.hovered = rect.contains(ctx->mousePosition)
		&& widgetIsHovered()
		&& !ctx->comboSlider.dragging
		&& ctx->comboSlider.pressedElementId == 0;
	state.clicked = false;

	if (ctx->widget.disabled)
	{
		state.hovered = false;
		state.pressed = false;
		return;
	}

	if (ctx->event.type == InputEvent::Type::MouseDown
		&& ctx->event.mouse.button == MouseButton::Left
		&& state.hovered
		&& ctx->isActiveLayer())
	{
		ctx->comboSlider.pressedElementId = elementId;
		state.pressed = true;
	}

	if (ctx->comboSlider.pressedElementId == elementId
		&& ctx->event.type == InputEvent::Type::MouseUp
		&& ctx->event.mouse.button == MouseButton::Left
		&& ctx->isActiveLayer())
	{
		state.clicked = rect.contains(ctx->mousePosition);
		state.pressed = false;
		ctx->comboSlider.pressedElementId = 0;
	}

	if (ctx->comboSlider.pressedElementId != elementId)
	{
		state.pressed = false;
	}
}

static ThemeElement::State* comboSliderElementState(ThemeElement& element, bool pressed, bool hovered, bool widgetFocused)
{
	if (pressed)
		return &element.getState(WidgetStateType::Pressed);
	if (widgetFocused)
		return &element.getState(WidgetStateType::Focused);
	if (hovered)
		return &element.getState(WidgetStateType::Hovered);

	return &element.normalState();
}

static bool comboSliderInternal(bool isInt, f32* value, f32 minVal, f32 maxVal, bool useRange, f32 stepsPerPixel, f32 arrowStep, const char* formatStr, u32 decimalPlaces = 4)
{
	auto& leftButtonElem = ctx->theme->getElement(WidgetElementId::ComboSliderLeftButton);
	auto& middleButtonElem = ctx->theme->getElement(WidgetElementId::ComboSliderMiddleButton);
	auto& rightButtonElem = ctx->theme->getElement(WidgetElementId::ComboSliderRightButton);
	auto& leftArrowElem = ctx->theme->getElement(WidgetElementId::ComboSliderLeftArrow);
	auto& rightArrowElem = ctx->theme->getElement(WidgetElementId::ComboSliderRightArrow);
	auto& rangeBarElem = ctx->theme->getElement(WidgetElementId::ComboSliderRangeBar);
	ctx->widget.changeEnded = false;
	f32 arrowZoneWidth = 0;
	auto& padding = widgetGetPadding();

	if (useRange)
	{
		ctx->widget.changeEnded = clampValue(*value, minVal, maxVal);
	}

	ctx->id = genId((void*)value);
	auto comboId = ctx->id;

	// every element gets its own id and state under the parent combo id,
	// so hovering or pressing one element doesn't affect the others
	idPush(comboId);
	auto leftButtonId = genId("comboSliderLeftButton");
	auto middleButtonId = genId("comboSliderMiddleButton");
	auto rightButtonId = genId("comboSliderRightButton");
	auto leftArrowId = genId("comboSliderLeftArrow");
	auto rightArrowId = genId("comboSliderRightArrow");
	auto rangeBarId = genId("comboSliderRangeBar");
	idPop();

	auto& elementStates = ctx->comboSlider.elementStates;
	auto& leftButton = elementStates[leftButtonId];
	auto& middleButton = elementStates[middleButtonId];
	auto& rightButton = elementStates[rightButtonId];
	auto& leftArrow = elementStates[leftArrowId];
	auto& rightArrow = elementStates[rightArrowId];
	auto& rangeBar = elementStates[rangeBarId];

	Rect leftBgRect;
	Rect rightBgRect;
	Rect middleBgRect;

	bool notEditingText = (ctx->comboSlider.editingText && ctx->comboSlider.id != ctx->id) || !ctx->comboSlider.editingText;

	if (notEditingText)
	{
		ctx->widget.customWidth = ctx->layout.width/ctx->scale;
		ctx->widget.hasCustomWidth = true;

		addWidget((leftButtonElem.normalState().height + padding.y * 2.0f) * ctx->scale);
		buttonBehavior();

		if (ctx->comboSlider.dragging && ctx->id == ctx->comboSlider.id)
		{
			ctx->widget.pressed = true;
		}

		auto leftButtonNormalState = &leftButtonElem.normalState();
		arrowZoneWidth = leftButtonNormalState->width;
		if (arrowZoneWidth <= 0)
		{
			arrowZoneWidth = leftArrowElem.normalState().image->width + padding.x * 2.0f;
		}
		arrowZoneWidth *= ctx->scale;

		leftBgRect.set(ctx->widget.rect.x, ctx->widget.rect.y, arrowZoneWidth, ctx->widget.rect.height);
		rightBgRect.set(ctx->widget.rect.right() - arrowZoneWidth, ctx->widget.rect.y, arrowZoneWidth, ctx->widget.rect.height);
		middleBgRect.set(ctx->widget.rect.x + arrowZoneWidth, ctx->widget.rect.y, ctx->widget.rect.width - arrowZoneWidth * 2.0f, ctx->widget.rect.height);
		if (middleBgRect.width < 0)
		{
			middleBgRect.width = 0;
		}

		// update each element's hover and press state
		comboSliderElementBehavior(leftButtonId, leftBgRect, leftButton);
		comboSliderElementBehavior(middleButtonId, middleBgRect, middleButton);
		comboSliderElementBehavior(rightButtonId, rightBgRect, rightButton);

		leftArrow.hovered = leftButton.hovered;
		leftArrow.pressed = leftButton.pressed;
		rightArrow.hovered = rightButton.hovered;
		rightArrow.pressed = rightButton.pressed;
		rangeBar.hovered = middleButton.hovered;
		rangeBar.pressed = middleButton.pressed;

		if (leftButton.clicked)
		{
			*value -= arrowStep;
			if (useRange) clampValue(*value, minVal, maxVal);
			ctx->widget.changeEnded = true;
		}
		else if (rightButton.clicked)
		{
			*value += arrowStep;
			if (useRange) clampValue(*value, minVal, maxVal);
			ctx->widget.changeEnded = true;
		}

		if (widgetIsHovered() || widgetIsPressed())
		{
			if (leftButton.hovered || rightButton.hovered
				|| leftButton.pressed || rightButton.pressed
				|| leftButton.clicked || rightButton.clicked
				|| ctx->comboSlider.pressedElementId == leftButtonId
				|| ctx->comboSlider.pressedElementId == rightButtonId)
				mouseCursorSetType(MouseCursorType::Arrow);
			else
				mouseCursorSetType(MouseCursorType::SizeWE);
		}

		if (middleButton.clicked && !ctx->comboSlider.dragging)
		{
			ctx->comboSlider.editingText = true;
			ctx->comboSlider.id = ctx->id;
			ctx->comboSlider.mouseWasDown = false;
			ctx->comboSlider.dragging = false;
			ctx->comboSlider.clickedToEditText = true;
			memset(ctx->comboSlider.text, ctx->comboSlider.maxTextSize, 0);
			stringFromF32(*value, ctx->comboSlider.text, ComboSliderState::maxTextSize, decimalPlaces);
			
			ctx->textInput.editNow = true;
			ctx->textInput.selectAllOnFocus = true;
			ctx->textInput.firstMouseDown = true;

			ctx->position.y -= ctx->spacing * ctx->scale + leftButtonElem.normalState().height;

			widgetSetNextFocused();
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
			widgetSetNextFocused();
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
			windowReleaseCapture();

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

		if (ctx->event.type == InputEvent::Type::MouseDown
			&& widgetIsHovered()
			&& !ctx->comboSlider.dragging
			&& ctx->isActiveLayer()
			&& !ctx->widget.disabled
			&& !leftButton.hovered
			&& !rightButton.hovered)
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
			windowSetCapture();
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
				ctx->comboSlider.hiddenCursorPos = ctx->settings.services.getAbsoluteMousePosition();
				ctx->settings.services.hideMouseCursor();
			}
		}

		if (ctx->comboSlider.dragging
			&& ctx->id == ctx->comboSlider.id
			&& ctx->isActiveLayer()
			&& !ctx->widget.disabled)
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
				clampValue(ctx->comboSlider.currentValue, minVal, maxVal);
				ctx->widget.changeEnded = true;
				percentFilled = 1.0f - (maxVal - ctx->comboSlider.currentValue) / (maxVal - minVal);
				*value = ctx->comboSlider.currentValue;
			}
			else
			{
				ctx->comboSlider.currentValue += deltaValue * stepsPerPixel;
				ctx->widget.changeEnded = true;
				*value = ctx->comboSlider.currentValue;
			}

			if (ctx->lastHoveredNativeWindow)
			{
				auto wndPos = ctx->settings.services.getWindowPosition(ctx->lastHoveredNativeWindow);
				auto wndSize = ctx->settings.services.getWindowSize(ctx->lastHoveredNativeWindow);
				auto absPos = ctx->settings.services.getAbsoluteMousePosition();
				const f32 margin = 2.0f;

				if (absPos.x <= wndPos.x + margin || absPos.x >= wndPos.x + wndSize.x - margin)
				{
					Point centerScreen(wndPos.x + wndSize.x * 0.5f, absPos.y);
					ctx->settings.services.setAbsoluteMousePosition(centerScreen);
					Point warpDelta = centerScreen - absPos;
					ctx->comboSlider.dragLastMousePos += warpDelta;
					ctx->mousePosition += warpDelta;
				}
			}
		}

		if (ctx->event.type == InputEvent::Type::MouseUp
			&& (ctx->comboSlider.dragging || ctx->comboSlider.mouseWasDown)
			&& ctx->isActiveLayer()
			&& ctx->comboSlider.id == ctx->id
			&& !ctx->widget.disabled)
		{
			if (ctx->comboSlider.dragging)
			{
				*value = ctx->comboSlider.currentValue;
				ctx->widget.changeEnded = true;
				ctx->settings.services.setAbsoluteMousePosition(ctx->comboSlider.hiddenCursorPos);
				ctx->settings.services.showMouseCursor();
			}
			ctx->comboSlider.dragging = false;
			ctx->comboSlider.mouseWasDown = false;
			ctx->comboSlider.id = 0;
			if (isInt) *value = roundf(*value);
			windowReleaseCapture();
		}

		auto leftButtonState = &leftButtonElem.normalState();
		auto middleButtonState = &middleButtonElem.normalState();
		auto rightButtonState = &rightButtonElem.normalState();
		auto leftArrowState = &leftArrowElem.normalState();
		auto rightArrowState = &rightArrowElem.normalState();
		auto rangeBarState = &rangeBarElem.normalState();

		if (ctx->widget.disabled)
		{
			leftButtonState = &leftButtonElem.getState(WidgetStateType::Disabled);
			middleButtonState = &middleButtonElem.getState(WidgetStateType::Disabled);
			rightButtonState = &rightButtonElem.getState(WidgetStateType::Disabled);
			leftArrowState = &leftArrowElem.getState(WidgetStateType::Disabled);
			rightArrowState = &rightArrowElem.getState(WidgetStateType::Disabled);
			rangeBarState = &rangeBarElem.getState(WidgetStateType::Disabled);
		}
		else
		{
			leftButtonState = comboSliderElementState(leftButtonElem, leftButton.pressed, leftButton.hovered, ctx->widget.focused);
			middleButtonState = comboSliderElementState(middleButtonElem, middleButton.pressed, middleButton.hovered, ctx->widget.focused);
			rightButtonState = comboSliderElementState(rightButtonElem, rightButton.pressed, rightButton.hovered, ctx->widget.focused);
			leftArrowState = comboSliderElementState(leftArrowElem, leftArrow.pressed, leftArrow.hovered, false);
			rightArrowState = comboSliderElementState(rightArrowElem, rightArrow.pressed, rightArrow.hovered, false);
			rangeBarState = comboSliderElementState(rangeBarElem, rangeBar.pressed, rangeBar.hovered, ctx->widget.focused);
		}

		Image* leftButtonImage = leftButtonState->image;
		Image* middleButtonImage = middleButtonState->image;
		Image* rightButtonImage = rightButtonState->image;
		Image* leftArrowImage = leftArrowState->image;
		Image* rightArrowImage = rightArrowState->image;
		Image* rangeBarImage = rangeBarState->image;

		if (ctx->widget.disabled)
		{
			if (!leftButtonImage) leftButtonImage = leftButtonElem.normalState().image;
			if (!middleButtonImage) middleButtonImage = middleButtonElem.normalState().image;
			if (!rightButtonImage) rightButtonImage = rightButtonElem.normalState().image;
			if (!leftArrowImage) leftArrowImage = leftArrowElem.normalState().image;
			if (!rightArrowImage) rightArrowImage = rightArrowElem.normalState().image;
			if (!rangeBarImage) rangeBarImage = rangeBarElem.normalState().image;
		}

		ctx->renderer.cmdSetColor(tintApply(leftButtonState->color, TintColorType::Body));
		ctx->renderer.cmdDrawImageBordered(leftButtonImage, leftButtonState->border, leftBgRect, ctx->scale);

		ctx->renderer.cmdSetColor(tintApply(middleButtonState->color, TintColorType::Body));
		ctx->renderer.cmdDrawImageBordered(middleButtonImage, middleButtonState->border, middleBgRect, ctx->scale);

		ctx->renderer.cmdSetColor(tintApply(rightButtonState->color, TintColorType::Body));
		ctx->renderer.cmdDrawImageBordered(rightButtonImage, rightButtonState->border, rightBgRect, ctx->scale);

		if (useRange)
		{
			ctx->renderer.cmdSetColor(tintApply(rangeBarState->color, TintColorType::Body));
			ctx->renderer.cmdDrawImageBordered(rangeBarImage, rangeBarState->border,
				{
					middleBgRect.x + middleButtonState->border * ctx->scale,
					middleBgRect.bottom() - middleButtonState->border * ctx->scale,
					(middleBgRect.width - middleButtonState->border * 2.0f * ctx->scale) * percentFilled,
					rangeBarState->height * ctx->scale,
				},
				ctx->scale);
		}

		// draw left arrow centered on its button
		{
			auto arrowX = leftBgRect.x + leftBgRect.width / 2.0f - (leftArrowImage->width * ctx->scale) / 2.0f;
			auto arrowY = leftBgRect.y + leftBgRect.height / 2.0f - (leftArrowImage->height * ctx->scale) / 2.0f;

			ctx->renderer.cmdSetColor(tintApply(leftArrowState->color, TintColorType::Body));
			ctx->renderer.cmdDrawImage(leftArrowImage,
				{
					arrowX,
					arrowY,
					leftArrowImage->width * ctx->scale,
					leftArrowImage->height * ctx->scale
				});
		}

		// draw right arrow centered on its button
		{
			auto arrowX = rightBgRect.x + rightBgRect.width / 2.0f - (rightArrowImage->width * ctx->scale) / 2.0f;
			auto arrowY = rightBgRect.y + rightBgRect.height / 2.0f - (rightArrowImage->height * ctx->scale) / 2.0f;

			ctx->renderer.cmdSetColor(tintApply(rightArrowState->color, TintColorType::Body));
			ctx->renderer.cmdDrawImage(rightArrowImage,
				{
					arrowX,
					arrowY,
					rightArrowImage->width * ctx->scale,
					rightArrowImage->height * ctx->scale
				});
		}

		static char outStr[ComboSliderState::maxTextSize] = { 0 };
		static char outStrFormatted[ComboSliderState::maxTextSize] = { 0 };
		char* str = nullptr;

		stringFromF32(*value, outStr, ComboSliderState::maxTextSize, decimalPlaces);

		if (!formatStr)
		{
			str = outStr;
		}
		else
		{
			sprintf(outStrFormatted, formatStr, *value);
			str = outStrFormatted;
		}

		ctx->renderer.cmdSetColor(tintApply(middleButtonState->textColor, TintColorType::Body));
		ctx->renderer.cmdDrawTextInBox(str, middleBgRect, HAlignType::Center, VAlignType::Center);
		widgetSetFocusable();
	}

	return ctx->widget.changeEnded;
}

bool comboSliderInt(i32* value, f32 stepsPerPixel, i32 arrowStep, const char* formatStr)
{
	f32 fVal = *value;

	idPush((void*)value);
	bool ret = comboSliderInternal(true, &fVal, 0, 0, false, stepsPerPixel, (f32)arrowStep, formatStr, 0);
	idPop();

	if (ret)
		*value = (i32)fVal;

	return ret;
}

bool comboSliderIntRanged(i32* value, i32 minVal, i32 maxVal, f32 stepsPerPixel, i32 arrowStep, const char* formatStr)
{
	f32 fVal = *value;
	idPush((void*)value);
	bool ret = comboSliderInternal(true, &fVal, (f32)minVal, (f32)maxVal, true, stepsPerPixel, (f32)arrowStep, formatStr, 0);
	idPop();

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
