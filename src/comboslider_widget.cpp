#include <algorithm>
#include <string.h>
#include "context.h"
#include "theme.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "util.h"

namespace hui
{
static bool comboSliderInternal(f32* value, f32 minVal, f32 maxVal, bool useRange, f32 stepsPerPixel, f32 arrowStep, const char* unitName)
{
	auto& bodyElem = ctx->theme->getElement(WidgetElementId::ComboSliderBody);
	auto& leftArrowElem = ctx->theme->getElement(WidgetElementId::ComboSliderLeftArrow);
	auto& rightArrowElem = ctx->theme->getElement(WidgetElementId::ComboSliderRightArrow);
	auto& rangeBarElem = ctx->theme->getElement(WidgetElementId::ComboSliderRangeBar);
	ctx->widget.changeEnded = false;
	bool arrowStepped = false;
	bool arrowHoveredLeft = false;
	bool arrowHoveredRight = false;

	if (useRange)
	{
		ctx->widget.changeEnded = clampValue(*value, minVal, maxVal);
	}

	ctx->id = genId((void*)value);

	bool notEditingText = (ctx->comboSlider.editingText && ctx->comboSlider.id != ctx->id) || !ctx->comboSlider.editingText;

	if (notEditingText)
	{
		addWidget(bodyElem.normalState().height * ctx->scale);
		buttonBehavior();

		if (ctx->comboSlider.dragging && ctx->id == ctx->comboSlider.id)
		{
			ctx->widget.pressed = true;
		}

		auto cursor = 0;

		if (isHovered() && ctx->mousePosition.x <= ctx->widget.rect.x + leftArrowElem.normalState().image->width)
		{
			arrowHoveredLeft = true;
		}
		else if (isHovered() && ctx->mousePosition.x >= ctx->widget.rect.right() - rightArrowElem.normalState().image->width)
		{
			arrowHoveredRight = true;
		}

		if (isClicked()
			&& ctx->mousePosition.x <= ctx->widget.rect.x + leftArrowElem.normalState().image->width)
		{
			*value -= arrowStep;
			arrowStepped = true;
			if (useRange) clampValue(*value, minVal, maxVal);
			ctx->widget.changeEnded = true;
		}
		else if (isClicked() && ctx->mousePosition.x >= ctx->widget.rect.right() - rightArrowElem.normalState().image->width)
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
			ctx->widget.focusedId = ctx->id;
			ctx->widget.focused = true;
			ctx->focusChanged = true;
			ctx->textInput.id = ctx->id;
			memset(ctx->comboSlider.text, 64, 0);
			toString(*value, ctx->comboSlider.text, ComboSliderState::maxTextSize);
			ctx->textInput.editNow = true;
			ctx->textInput.selectAllOnFocus = true;
			ctx->textInput.firstMouseDown = true;
			forceRepaint();
			ctx->position.y -= ctx->spacing * ctx->scale + bodyElem.normalState().height;
			setNextFocused();
			textInput(ctx->comboSlider.text, ComboSliderState::maxTextSize, TextInputValueMode::NumericOnly);
			ctx->widget.focusedId = ctx->id;
		}
	}
	else
	if (ctx->comboSlider.editingText && ctx->comboSlider.id == ctx->id)
	{
		textInput(ctx->comboSlider.text, ComboSliderState::maxTextSize, TextInputValueMode::NumericOnly);

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
				*value += deltaValue * stepsPerPixel;
				ctx->widget.changeEnded = clampValue(*value, minVal, maxVal);
				percentFilled = 1.0f - (maxVal - *value) / (maxVal - minVal);
			}
			else
			{
				*value += deltaValue * stepsPerPixel;
				ctx->widget.changeEnded = true;
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
			//TODO: releaseCapture();
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

		ctx->renderer->cmdSetColor(applyTint(bodyElemState->color, TintColorType::Body));
		ctx->renderer->cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->scale);
		
		if (useRange)
		{
			ctx->renderer->cmdSetColor(applyTint(rangeBarElemState->color, TintColorType::Body));
			ctx->renderer->cmdDrawImageBordered(rangeBarElemState->image, rangeBarElemState->border,
				{
					ctx->widget.rect.x + bodyElemState->border * ctx->scale,
					ctx->widget.rect.bottom() - bodyElemState->border * ctx->scale,
					(valueWidth - bodyElemState->border * 2.0f * ctx->scale) * percentFilled,
					rangeBarElemState->height * ctx->scale,
				},
				ctx->scale);
		}

		ctx->renderer->cmdSetColor(applyTint(leftArrowElemState->color, TintColorType::Body));

		// dial down the height, since its already global scaled
		auto arrowY = ((ctx->widget.rect.height / ctx->scale - leftArrowElemState->image->rect.height) / 2.0f + (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->scale;

		ctx->renderer->cmdDrawImage(leftArrowElemState->image,
			{
				ctx->widget.rect.x + bodyElemState->border + (ctx->widget.pressed ? 1.0f : 0.0f) * ctx->scale,
				ctx->widget.rect.top() + arrowY,
				leftArrowElemState->image->rect.width * ctx->scale,
				leftArrowElemState->image->rect.height * ctx->scale
			});

		ctx->renderer->cmdSetColor(applyTint(rightArrowElemState->color, TintColorType::Body));

		// dial down the height, since its already global scaled
		arrowY = ((ctx->widget.rect.height / ctx->scale - rightArrowElemState->image->rect.height) / 2.0f + (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->scale;

		ctx->renderer->cmdDrawImage(rightArrowElemState->image,
			{
				ctx->widget.rect.right() - bodyElemState->border - (rightArrowElemState->image->rect.width - (ctx->widget.pressed ? 1.0f : 0.0f)) * ctx->scale,
				ctx->widget.rect.top() + arrowY,
				rightArrowElemState->image->rect.width * ctx->scale,
				rightArrowElemState->image->rect.height * ctx->scale
			});

		static char outStr[ComboSliderState::maxTextSize] = { 0 };
		static char outStrWithUnitName[ComboSliderState::maxTextSize] = { 0 };
		char* str = nullptr;

		toString(*value, outStr, ComboSliderState::maxTextSize);

		if (!unitName)
		{
			str = outStr;
		}
		else
		{
			sprintf(outStrWithUnitName, "%s %s", outStr, unitName);
			str = outStrWithUnitName;
		}

		ctx->renderer->cmdSetColor(applyTint(bodyElemState->textColor, TintColorType::Body));
		ctx->renderer->cmdDrawTextInBox(str, ctx->widget.rect, HAlignType::Center, VAlignType::Center);
		setFocusable();
	}

	return ctx->widget.changeEnded;
}

bool comboSliderFloat(f32* value, f32 stepsPerPixel, f32 arrowStep, const char* unitName)
{
	return comboSliderInternal(value, 0, 0, false, stepsPerPixel, arrowStep, unitName);
}

bool comboSliderFloatRanged(f32* value, f32 minVal, f32 maxVal, f32 stepsPerPixel, f32 arrowStep, const char* unitName)
{
	return comboSliderInternal(value, minVal, maxVal, true, stepsPerPixel, arrowStep, unitName);
}

}
