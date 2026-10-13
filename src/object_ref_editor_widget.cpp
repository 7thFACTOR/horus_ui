#include <algorithm>
#include "context.h"
#include "theme.h"
#include "util.h"

namespace hui
{
bool objectRefEditor(const char* id, HImage targetImg, HImage clearImg, HImage iconImg, const char* objectTypeName, const char* valueAsString, u32 objectType, void** outObject, bool* objectValueWasModified, u32 refCount, const char** refNames, void** refValues, f32 iconSize, const char* indeterminateStr)
{
	idPush(id);
	bool returnValue = false;
	bool changeEnded = false;

	if (objectValueWasModified)
		*objectValueWasModified = false;

	WidgetElementInfo targetElemInfo;

	hui::themeGetUserWidgetElementInfo("objectRefEditorBody", WidgetStateType::Normal, targetElemInfo);

	if (!targetImg)
	{
		WidgetElementInfo info;
		themeGetUserWidgetElementInfo("objectRefEditorTargetButton", WidgetStateType::Normal, info);
		targetImg = info.image;
	}

	if (!clearImg)
	{
		WidgetElementInfo info;
		themeGetUserWidgetElementInfo("objectRefEditorClearButton", WidgetStateType::Normal, info);
		clearImg = info.image;
	}

	f32 btnSize = targetElemInfo.height;
	f32 spacingPx = ctx->sameLine.spacing * ctx->scale;
	f32 btnSizePx = btnSize * ctx->scale;

	f32 contentHeightPx = btnSizePx;
	f32 iconDisplaySizePx = 0;
	if (iconImg)
	{
		if (iconSize > 0)
		{
			iconDisplaySizePx = iconSize * ctx->scale;
			contentHeightPx = iconDisplaySizePx;
		}
		else
		{
			iconDisplaySizePx = contentHeightPx;
		}
	}

	f32 boxWidthPx = ctx->layout.width - btnSizePx * (*outObject ? 2 : 1);

	widgetSetNextWidth(boxWidthPx / ctx->scale);
	ctx->setLabelAndId(id);
	addWidget(contentHeightPx / ctx->scale);
	buttonBehavior();
	ctx->sameLine.maxHeight = std::max(ctx->sameLine.maxHeight, contentHeightPx);

	auto& bodyElem = ctx->theme->getElement(WidgetElementId::TextInputBody);
	auto bodyState = &bodyElem.normalState();
	if (ctx->widget.disabled)
		bodyState = &bodyElem.getState(WidgetStateType::Disabled);
	else if (ctx->widget.pressed)
		bodyState = &bodyElem.getState(WidgetStateType::Pressed);
	else if (ctx->widget.focused)
		bodyState = &bodyElem.getState(WidgetStateType::Focused);
	else if (ctx->widget.hovered)
		bodyState = &bodyElem.getState(WidgetStateType::Hovered);

	if (ctx->widget.visible)
	{
		ctx->renderer.cmdSetColor(tintApply(bodyState->color, TintColorType::Body));
		ctx->renderer.cmdDrawImageBordered(bodyState->image, bodyState->border, ctx->widget.rect, ctx->scale);

		Rect textRect = ctx->widget.rect;
		f32 borderPx = bodyState->border * ctx->scale;
		f32 paddingPx = 4 * ctx->scale;

		textRect.x += borderPx;
		textRect.y += borderPx;
		textRect.width -= borderPx * 2.0f;
		textRect.height -= borderPx * 2.0f;

		if (iconImg && *outObject)
		{
			f32 iconDrawSizePx = std::min(iconDisplaySizePx, textRect.height);

			Image* iconPtr = (Image*)iconImg;
			Rect iconRect = {
				textRect.x + paddingPx,
				textRect.y + (textRect.height - iconDrawSizePx) / 2.0f,
				iconDrawSizePx,
				iconDrawSizePx
			};
			ctx->renderer.cmdSetColor(Color::white);
			ctx->renderer.cmdDrawImage(iconPtr, iconRect);
			textRect.x += paddingPx + iconDrawSizePx + paddingPx;
			textRect.width -= paddingPx + iconDrawSizePx + paddingPx;
		}

		Color textColor;
		Font* textFont;
		if (*outObject)
		{
			WidgetElementInfo info;
			themeGetUserWidgetElementInfo("objectRefEditorBody", WidgetStateType::Pressed, info);
			textColor = info.textColor;
			textFont = info.font ? (Font*)info.font : bodyState->font;
		}
		else
		{
			textColor = bodyState->textColor;
			textFont = bodyState->font;
		}

		std::string text;
		if (!*outObject)
		{
			if (indeterminateStr)
			{
				text = indeterminateStr;

				auto& indeterminateTextElem = ctx->theme->getElement(WidgetElementId::TextInputDefaultText);
				auto& indeterminateState = ctx->widget.disabled
					? indeterminateTextElem.getState(WidgetStateType::Disabled)
					: indeterminateTextElem.normalState();

				textColor = indeterminateState.textColor;
			}
			else
			{
				text += "None (";
				text += objectTypeName;
				text += ")";
			}
		}
		else
		{
			text = valueAsString;
		}

		ctx->renderer.cmdSetColor(tintApply(textColor, TintColorType::Text));
		ctx->renderer.cmdSetFont(textFont);
		ctx->renderer.cmdDrawTextInBox(text.c_str(), textRect, HAlignType::Left, VAlignType::Center, true);
	}

	widgetSetFocusable();

	if (ctx->widget.doubleClicked)
		returnValue = true;

	if (ctx->widget.focused
		&& *outObject
		&& ctx->event.type == InputEvent::Type::Key
		&& (ctx->event.key.code == KeyCode::Delete || ctx->event.key.code == KeyCode::Backspace)
		&& ctx->event.key.down)
	{
		*outObject = nullptr;

		if (objectValueWasModified)
			*objectValueWasModified = true;

		changeEnded = true;
		forceRepaint();
	}

	if (dragDropGetObjectType() == objectType)
	{
		dragDropAllow();
	}

	if (dragDropDroppedOnWidget() && dragDropGetObjectType() == objectType)
	{
		*outObject = dragDropGetObject();
		dragDropEnd();

		if (objectValueWasModified)
			*objectValueWasModified = true;

		changeEnded = true;
		forceRepaint();
	}

	sameLine(-ctx->sameLine.spacing, 0);

	if (targetImg)
	{
		returnValue = imageButton(targetImg, btnSize, btnSize);
		tooltip("Select reference");
	}
	else
	{
		ctx->widget.customWidth = btnSize;
		ctx->widget.hasCustomWidth = true;
		ctx->setLabelAndId("targetBtn");
		addWidget(btnSize);
		buttonBehavior();

		auto& btnBody = ctx->theme->getElement(WidgetElementId::ImageButtonBody);
		auto state = &btnBody.normalState();
		if (ctx->widget.disabled)
			state = &btnBody.getState(WidgetStateType::Disabled);
		else if (ctx->widget.pressed)
			state = &btnBody.getState(WidgetStateType::Pressed);
		else if (ctx->widget.focused)
			state = &btnBody.getState(WidgetStateType::Focused);
		else if (ctx->widget.hovered)
			state = &btnBody.getState(WidgetStateType::Hovered);

		if (ctx->widget.visible)
		{
			auto bodyImage = state->image;
			if (!bodyImage && ctx->widget.disabled)
				bodyImage = btnBody.normalState().image;

			ctx->renderer.cmdSetColor(tintApply(state->color, TintColorType::Body));
			ctx->renderer.cmdDrawImageBordered(bodyImage, state->border, ctx->widget.rect, ctx->scale);

			ctx->renderer.cmdSetColor(tintApply(state->textColor, TintColorType::Text));
			ctx->renderer.cmdSetFont(state->font);
			ctx->renderer.cmdDrawTextInBox("...", ctx->widget.rect, HAlignType::Center, VAlignType::Center, true);
		}

		widgetSetFocusable();
		if (widgetIsClicked())
			forceRepaint();

		returnValue = ctx->widget.clicked;
		tooltip("Select reference");
	}

	Rect targetBtnRect = ctx->widget.rect;
	WidgetId targetWidgetId = ctx->id;

	if (refCount > 0 && returnValue)
	{
		ctx->dropdown.active = !ctx->dropdown.active;
		if (ctx->dropdown.active)
			ctx->dropdown.id = targetWidgetId;
		else
			ctx->dropdown.id = 0;
	}

	if (*outObject)
	{
		sameLine(-ctx->sameLine.spacing, 0);

		if (clearImg)
		{
			tintPush(Color::darkRed);

			if (imageButton(clearImg, btnSize, btnSize))
			{
				*outObject = nullptr;

				if (objectValueWasModified)
					*objectValueWasModified = true;

				changeEnded = true;
			}

			tintPop();
			tooltip("Clear reference");
		}
		else
		{
			ctx->widget.customWidth = btnSize;
			ctx->widget.hasCustomWidth = true;
			ctx->setLabelAndId("clearBtn");
			addWidget(btnSize);
			buttonBehavior();

			auto& btnBody = ctx->theme->getElement(WidgetElementId::ImageButtonBody);
			auto state = &btnBody.normalState();
			if (ctx->widget.disabled)
				state = &btnBody.getState(WidgetStateType::Disabled);
			else if (ctx->widget.pressed)
				state = &btnBody.getState(WidgetStateType::Pressed);
			else if (ctx->widget.focused)
				state = &btnBody.getState(WidgetStateType::Focused);
			else if (ctx->widget.hovered)
				state = &btnBody.getState(WidgetStateType::Hovered);

			if (ctx->widget.visible)
			{
				auto bodyImage = state->image;
				if (!bodyImage && ctx->widget.disabled)
					bodyImage = btnBody.normalState().image;

				ctx->renderer.cmdSetColor(tintApply(Color::darkRed, TintColorType::Body));
				ctx->renderer.cmdDrawImageBordered(bodyImage, state->border, ctx->widget.rect, ctx->scale);

				ctx->renderer.cmdSetColor(tintApply(Color::darkRed, TintColorType::Text));
				ctx->renderer.cmdSetFont(state->font);
				ctx->renderer.cmdDrawTextInBox("X", ctx->widget.rect, HAlignType::Center, VAlignType::Center, true);
			}

			widgetSetFocusable();
			if (widgetIsClicked())
				forceRepaint();

			if (ctx->widget.clicked)
			{
				*outObject = nullptr;

				if (objectValueWasModified)
					*objectValueWasModified = true;

				changeEnded = true;
			}

			tooltip("Clear reference");
		}
	}

	if (refCount > 0 && ctx->dropdown.active && targetWidgetId == ctx->dropdown.id)
	{
		bool selectedNewItem = false;
		popupBegin("##refPopup", 200, PopupFlags::CustomPosition, targetBtnRect.bottomLeft());

		for (u32 i = 0; i < refCount; i++)
		{
			if (selectable(refNames[i], refValues && refValues[i] == *outObject ? SelectableFlags::Selected : SelectableFlags::Normal))
			{
				*outObject = refValues[i];

				if (objectValueWasModified)
					*objectValueWasModified = true;

				changeEnded = true;
				selectedNewItem = true;
			}
		}

		if (selectedNewItem || popupMustClose())
		{
			popupClose();
			ctx->dropdown.active = false;
		}

		popupEnd();
	}

	ctx->widget.changeEnded = changeEnded;

	idPop();

	return returnValue;
}

}
