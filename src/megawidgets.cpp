#include <float.h>
#include "context.h"
#include "theme.h"
#include "util.h"

namespace hui
{
static bool vecEditorInternal(f64& x, f64& y, f64& z, f64 scrollStep, bool useZ, VectorEditorFlags flags, u32 precision, const char* indeterminateStr)
{
	// the editing state is shared for all vec editors, so tie it to the
	// editor instance that started it
	WidgetId editorId = ctx->idStack.back();
	auto& editIndeterminate = ctx->vecEditor;

	bool modified = false;
	bool changedEndedX = false;
	bool changedEndedY = false;
	bool changedEndedZ = false;

	char fmt[16];
	snprintf(fmt, sizeof(fmt), "%%.%ug", precision);

	f32 spacing = ctx->sameLine.spacing * ctx->scale;
	u32 axisCount = useZ ? 3 : 2;

	auto imgElem = ctx->theme->userElements["axisBoxXImage"];
	f32 imgWidthPx = ((Image*)imgElem->normalState().image)->width * ctx->scale;

	// each axis inserts two sameLine spacings (image->input, input->next image),
	// the last axis only one, so a 3-axis editor uses 5 spacings in total.
	f32 totalWidth = (f32)axisCount * imgWidthPx + ((f32)axisCount * 2.0f - 1.0f) * spacing;
	f32 inputWidthPx = (ctx->layout.width - totalWidth) / (f32)axisCount;
	f32 inputWidthUnscaled = inputWidthPx / ctx->scale;

	// handle MouseUp outside the per-axis loop so it only fires once
	// and doesn't interfere with the wrong axis iteration
	if (editIndeterminate.draggingValue
		&& hui::inputEventGet().type == hui::InputEvent::Type::MouseUp)
	{
		editIndeterminate.draggingValue = false;
		editIndeterminate.draggedId = 0;
		ctx->settings.services.setAbsoluteMousePosition(editIndeterminate.hiddenCursorPos);
		ctx->settings.services.showMouseCursor();
		hui::windowReleaseCapture();
		changedEndedX = changedEndedY = changedEndedZ = true;
	}

	for (u32 i = 0; i < axisCount; i++)
	{
		if (i > 0)
			hui::sameLine();

		const char* imgName = (i == 0) ? "axisBoxXImage" : (i == 1) ? "axisBoxYImage" : "axisBoxZImage";
		const char* inputId = (i == 0) ? "axisEditX" : (i == 1) ? "axisEditY" : "axisEditZ";
		char* strAxis = (i == 0) ? editIndeterminate.strX : (i == 1) ? editIndeterminate.strY : editIndeterminate.strZ;
		f64* val = (i == 0) ? &x : (i == 1) ? &y : &z;
		bool* changeEnded = (i == 0) ? &changedEndedX : (i == 1) ? &changedEndedY : &changedEndedZ;

		// indeterminate component: the indeterminate text is
		// shown as the input's hint (defaultText), the text stays empty until
		// the user types; dragging is disabled and an empty commit returns FLT_MAX
		bool indeterminateAxis = (i == 0) ? has(flags, VectorEditorFlags::IndeterminateX)
			: (i == 1) ? has(flags, VectorEditorFlags::IndeterminateY)
			: has(flags, VectorEditorFlags::IndeterminateZ);
		bool editingIndeterminate = (editIndeterminate.editingIndeterminateEditor == editorId) && editIndeterminate.editingIndeterminate[i];
		const char* hint = (indeterminateAxis && !editingIndeterminate) ? (indeterminateStr ? indeterminateStr : "Indeterminate") : nullptr;

		if (indeterminateAxis && !editingIndeterminate)
			strAxis[0] = 0;
		else if (!editingIndeterminate)
			snprintf(strAxis, VectorEditorState::maxStrSize, fmt, *val);

		auto elem = ctx->theme->userElements[imgName];
		hui::image(elem->normalState().image, 22, hui::HAlignType::Left);
		WidgetId imageWidgetId = ctx->id;
		bool imageHovered = hui::widgetIsHovered();
		bool imagePressed = hui::widgetIsPressed();
		hui::sameLine();
		hui::widgetSetNextWidth(inputWidthUnscaled);
		auto tiFlags = has(flags, VectorEditorFlags::AutoSelectAll) ? TextInputFlags::AutoSelectAll : TextInputFlags::None;
		modified = hui::textInput(inputId, strAxis, VectorEditorState::maxStrSize, tiFlags, hint) || modified;

		// clicking into an indeterminate field starts editing; the indeterminate text is a
		// hint, so the field is already empty and ready for input
		if (indeterminateAxis && !editingIndeterminate && ctx->widget.focused)
		{
			editIndeterminate.editingIndeterminateEditor = editorId;
			editIndeterminate.editingIndeterminate[i] = true;
			editIndeterminate.indeterminateText[i][0] = 0;
		}

		// dragging is disabled for indeterminate components
		if (!indeterminateAxis
			&& (imageHovered || (editIndeterminate.draggingValue && editIndeterminate.draggedId == imageWidgetId)))
		{
			hui::mouseCursorSetType(hui::MouseCursorType::SizeWE);

			if (imagePressed && !editIndeterminate.draggingValue)
			{
				editIndeterminate.draggingValue = true;
				editIndeterminate.draggedId = imageWidgetId;
				editIndeterminate.lastMousePos = ctx->mousePosition;
				editIndeterminate.hiddenCursorPos = ctx->settings.services.getAbsoluteMousePosition();
				ctx->settings.services.hideMouseCursor();
				ctx->widget.pressed = false;
				hui::windowSetCapture();
			}
		}

		if (!indeterminateAxis
			&& editIndeterminate.draggingValue
			&& editIndeterminate.draggedId == imageWidgetId)
		{
			*val = atof(strAxis);
			f32 dx = ctx->mousePosition.x - editIndeterminate.lastMousePos.x;
			f32 unitPerPixel = (f32)scrollStep;

			*val += (f64)dx * unitPerPixel;
			editIndeterminate.lastMousePos = ctx->mousePosition;

			// infinite drag: warp cursor to center of window when hitting edges
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
					editIndeterminate.lastMousePos += warpDelta;
					ctx->mousePosition += warpDelta;
				}
			}

			snprintf(strAxis, VectorEditorState::maxStrSize, fmt, *val);
			modified = true;
		}

		if (widgetIsChangeEnded())
			*changeEnded = true;

		// an indeterminate field finalizes its value when it loses focus: text that was
		// typed into the session becomes a value, anything else is indeterminate
		// (FLT_MAX). typed text is tracked in indeterminateText because strAxis is one
		// of the shared strX/strY/strZ buffers that other editors overwrite
		if (indeterminateAxis && editingIndeterminate)
		{
			if (ctx->widget.focused && ctx->id == ctx->textInput.id && strcmp(strAxis, editIndeterminate.indeterminateText[i]) != 0)
			{
				strncpy(editIndeterminate.indeterminateText[i], strAxis, VectorEditorState::maxStrSize - 1);
				editIndeterminate.indeterminateText[i][VectorEditorState::maxStrSize - 1] = 0;
			}

			if (!ctx->widget.focused)
			{
				editIndeterminate.editingIndeterminate[i] = false;
				editIndeterminate.editingIndeterminateEditor = 0;

				if (editIndeterminate.indeterminateText[i][0] == 0)
					*val = FLT_MAX;
				else
					*val = atof(editIndeterminate.indeterminateText[i]);

				modified = true;
			}
		}
		else if (modified && !indeterminateAxis)
		{
			*val = atof(strAxis);
		}
	}

	ctx->widget.changeEnded = changedEndedX || changedEndedY || changedEndedZ;

	return modified;
}

bool vec3Editor(const char* id, f64& x, f64& y, f64& z, f64 scrollStep, VectorEditorFlags flags, u32 precision, const char* indeterminateStr)
{
	idPush(id);
	bool ret = vecEditorInternal(x, y, z, scrollStep, true, flags, precision, indeterminateStr);
	idPop();

	return ret;
}

bool vec3Editor(const char* id, f32& x, f32& y, f32& z, f32 scrollStep, VectorEditorFlags flags, u32 precision, const char* indeterminateStr)
{
	idPush(id);
	f64 xx = x, yy = y, zz = z;

	auto ret = vecEditorInternal(xx, yy, zz, scrollStep, true, flags, precision, indeterminateStr);

	x = (f32)xx;
	y = (f32)yy;
	z = (f32)zz;

	idPop();

	return ret;
}

bool vec2Editor(const char* id, f64& x, f64& y, f64 scrollStep, VectorEditorFlags flags, u32 precision, const char* indeterminateStr)
{
	idPush(id);
	
	f64 zz = 0;
	bool ret = vecEditorInternal(x, y, zz, scrollStep, false, flags, precision, indeterminateStr);
	idPop();

	return ret;
}

bool vec2Editor(const char* id, f32& x, f32& y, f32 scrollStep, VectorEditorFlags flags, u32 precision, const char* indeterminateStr)
{
	idPush(id);
	f64 xx = x, yy = y, zz = 0;
	auto ret = vecEditorInternal(xx, yy, zz, scrollStep, false, flags, precision, indeterminateStr);

	x = (f32)xx;
	y = (f32)yy;
	idPop();

	return ret;
}

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

	// editor body (left side, fills remaining width)
	widgetSetNextWidth(boxWidthPx / ctx->scale);
	ctx->setLabelAndId(id);
	// contentHeightPx is already scaled, so normalize it here
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

		// keep the content inside the body's border so the icon and text don't
		// overlap the beveled edge
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

				// use the text input's default/hint text style so the indeterminate
				// text reads as a hint rather than a real object name
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

	// double-click on body triggers reference selection
	if (ctx->widget.doubleClicked)
		returnValue = true;

	// clear reference on Delete/Backspace key when focused
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

	// drop handling (ctx->widget.hovered is still for the editor)
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

	// target button on the right
	sameLine(-ctx->sameLine.spacing, 0);

	if (targetImg)
	{
		returnValue = imageButton(targetImg, btnSize, btnSize);
		tooltip("Select reference");
	}
	else
	{
		// draw "..." text button when no target image
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

	// save target button position and id for popup
	Rect targetBtnRect = ctx->widget.rect;
	WidgetId targetWidgetId = ctx->id;

	// toggle reference selection popup when target button clicked
	if (refCount > 0 && returnValue)
	{
		ctx->dropdown.active = !ctx->dropdown.active;
		if (ctx->dropdown.active)
			ctx->dropdown.id = targetWidgetId;
		else
			ctx->dropdown.id = 0;
	}

	// clear button next to target (only shown when a ref is set)
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
			// draw "X" text button when no clear image
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

	// reference selection popup
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