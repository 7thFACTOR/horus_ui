#include <string.h>
#include <algorithm>
#include "context.h"
#include "theme.h"
#include "renderer.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool textInput(
	const char* id,
	char* text,
	u32 maxLength,
	TextInputFlags flags,
	const char* defaultText,
	HImage img,
	bool password,
	const char* passwordChar)
{
	auto bodyElem = &ctx->theme->getElement(WidgetElementId::TextInputBody);
	auto& bodyTextCaretElemState = ctx->theme->getElement(WidgetElementId::TextInputCaret).normalState();
	auto& bodyTextSelectionElemState = ctx->theme->getElement(WidgetElementId::TextInputSelection).normalState();
	auto& bodyTextDefaultElemState = ctx->theme->getElement(WidgetElementId::TextInputDefaultText).normalState();
	auto& bodyTextFilterClearImageElem = ctx->theme->getElement(WidgetElementId::TextInputFilterClearImage);
	auto& padding = widgetGetPadding();

	if (!ctx->widget.hasNextWidth)
	{
		ctx->widget.customWidth = ctx->layout.width/ctx->scale;
		ctx->widget.hasCustomWidth = true;
	}

	// use ptr as id
	ctx->id = genId(id);
	addWidget(bodyElem->normalState().height);

	// always run button behavior so hover state is updated even when focusChanged is set
	buttonBehavior();

	if (ctx->focusChanged
		&& ctx->id != ctx->widget.focusedId)
	{
		ctx->widget.changeEnded = true;
	}

	// show clear image only when defaultText is provided and the current text is not empty.
	bool showClearImage = defaultText && strcmp(text, "") && strcmp(defaultText, "");

	// compute clear-filter hit rect here as well so the clear image can be hovered
	// even when the field is not focused (processEvent is only called for active editor).
	if (showClearImage)
	{
		Rect clearFilterRc = ctx->widget.rect;
		auto& clearElemState = bodyTextFilterClearImageElem.normalState();
		clearFilterRc.x = ctx->widget.rect.right() - (clearElemState.border + padding.x) * ctx->scale - clearElemState.image->width * ctx->scale;
		clearFilterRc.width = clearElemState.image->width * ctx->scale;
		clearFilterRc.height = clearElemState.image->height * ctx->scale;

		// contextUpdate hover flag based on global mouse position
		ctx->textInput.clearFilterHovered = clearFilterRc.contains(ctx->mousePosition) && ctx->hoveringThisWindow;
	}
	else
	{
		ctx->textInput.clearFilterHovered = false;
	}

	if (ctx->widget.pressed && ctx->textInput.clearFilterHovered)
	{
		if (text)
		{
			text[0] = 0;

			if (ctx->textInput.id == ctx->id)
			{
				ctx->textInput.text.clear();
				ctx->textInput.caretPosition = 0;
				ctx->textInput.selectionActive = false;
			}

			ctx->textInput.textChanged = true;
			forceRepaint();
		}
	}

	auto bodyElemState = &bodyElem->normalState();
	bool isEditingThis =
		ctx->id == ctx->textInput.id
		&& ctx->widget.focused
		&& ctx->isActiveLayer();

	ctx->textInput.themeElement = bodyElem;

	if (ctx->widget.disabled)
	{
		bodyElemState = &bodyElem->getState(WidgetStateType::Disabled);
	}
	else if (ctx->widget.focused)
	{
		bodyElemState = &bodyElem->getState(WidgetStateType::Focused);
	}

	auto clipRect = Rect(
		ctx->widget.rect.x + (bodyElemState->border + padding.x) * ctx->scale,
		ctx->widget.rect.y + (bodyElemState->border + padding.y) * ctx->scale,
		ctx->widget.rect.width - (bodyElemState->border + padding.x) * 2.0f * ctx->scale,
		ctx->widget.rect.height - (bodyElemState->border + padding.y) * 2.0f * ctx->scale);

	const size_t maxHiddenCharLen = 1024;
	static char hiddenPwdText[maxHiddenCharLen] = "";
	bool isEmptyText = false;
	char* textToDraw = (char*)text;
	Utf32String pwdStr;

	if (password)
	{
		ctx->settings.services.utf8To32(passwordChar, pwdStr);
	}

	ctx->textInput.editNow = false;

	if (ctx->event.type == InputEvent::Type::Key
		&& ctx->event.key.down
		&& ctx->widget.focused)
	{
		if (ctx->event.key.code == KeyCode::Enter)
		{
			if (!ctx->textInput.id)
			{
				ctx->textInput.id = ctx->id;
				isEditingThis = true;
				ctx->textInput.editNow = true;
				ctx->textInput.selectAllOnFocus = true;
			}
			else
			{
				ctx->textInput.id = 0;
				ctx->textInput.editNow = false;
				isEditingThis = false;
				ctx->widget.focusedId = 0;
				ctx->widget.changeEnded = true;
				ctx->widget.pressed = false;
			}
		}
		else if (ctx->event.key.code == KeyCode::Esc)
		{
			ctx->textInput.id = 0;
			ctx->textInput.editNow = false;
			isEditingThis = false;
			ctx->widget.focusedId = 0;
			ctx->widget.changeEnded = true;
			ctx->widget.pressed = false;
		}
	}

	if (ctx->focusChanged
		&& ctx->id == ctx->widget.focusedId)
	{
		ctx->textInput.editNow = true;
		isEditingThis = 0 != ctx->textInput.id;
		ctx->textInput.selectAllOnFocus = true;

		if (isEditingThis)
		{
			ctx->textInput.id = ctx->id;
		}
	}

	if (ctx->widget.pressed
		&& ctx->id != ctx->textInput.id)
	{
		ctx->textInput.id = ctx->id;
		ctx->textInput.editNow = true;
		isEditingThis = true;
		ctx->textInput.selectAllOnFocus = true;
		ctx->textInput.firstMouseDown = true;
	}

	if (ctx->textInput.editNow)
	{
		ctx->textInput.password = password;
		ctx->textInput.passwordCharUnicode = pwdStr;
		ctx->textInput.rect = ctx->widget.rect;
		ctx->textInput.clipRect = clipRect;
		ctx->textInput.maxTextLength = maxLength;
		ctx->textInput.selectionActive = false;
		ctx->textInput.flags = flags;
		ctx->textInput.scrollOffset = 0;
		ctx->settings.services.utf8To32(text, ctx->textInput.text);
		ctx->settings.services.utf8To32(defaultText, ctx->textInput.defaultText);

		if (ctx->textInput.selectAllOnFocus && has(flags, TextInputFlags::AutoSelectAll))
		{
			ctx->textInput.selectAll();
		}
		else
		{
			ctx->textInput.selectionActive = true;
			ctx->textInput.caretPosition = ctx->textInput.getCharIndexAtX(ctx->mousePosition.x);
			ctx->textInput.selectionBegin = ctx->textInput.caretPosition;
			ctx->textInput.selectionEnd = ctx->textInput.caretPosition;
			ctx->textInput.mouseDown = true;
			ctx->textInput.mouseDownSelectionBegin = ctx->textInput.caretPosition;
			ctx->textInput.mouseMoved = false;
			ctx->textInput.selectingWithMouse = false;
			ctx->textInput.computeScrollAmount();
			windowSetCapture();
		}

		Rect rc;

		rc.x = ctx->widget.rect.x;
		rc.y = ctx->widget.rect.y;
		rc.width = ctx->widget.rect.width;
		rc.height = ctx->widget.rect.height;
		ctx->settings.services.startTextInput(ctx->lastHoveredNativeWindow, rc);
		bodyElemState = &bodyElem->getState(WidgetStateType::Focused);
		forceRepaint();
	}

	// if hovering the clear button prefer Arrow, otherwise show I-beam when hovering the text area.
	if (ctx->textInput.clearFilterHovered)
	{
		mouseCursorSetType(MouseCursorType::Arrow);
	}
	else if (ctx->widget.hovered)
	{
		mouseCursorSetType(MouseCursorType::IBeam);
	}

	ctx->renderer.cmdSetColor(bodyElemState->color);

	Image* bodyImage = bodyElemState->image;

	if (ctx->widget.disabled && !bodyImage)
	{
		bodyImage = bodyElem->normalState().image;
	}

	ctx->renderer.cmdDrawImageBordered(bodyImage, bodyElemState->border, ctx->widget.rect, ctx->scale);
	ctx->renderer.cmdSetColor(bodyElemState->textColor);
	ctx->renderer.cmdSetFont(bodyElemState->font);
	ctx->renderer.pushClipRect(clipRect);

	if (isEditingThis)
	{
		i32 offs = ctx->textInput.caretPosition;

		if (ctx->textInput.caretPosition > ctx->textInput.text.size())
			offs = ctx->textInput.text.size() - 1;

		FontTextSize textToCursorSize;
		Utf32String textToCursor;

		textToCursor = Utf32String(
			ctx->textInput.text.begin(), ctx->textInput.text.begin() + offs);

		if (!password)
		{
			textToCursorSize = bodyElemState->font->computeTextSize(textToCursor);
		}
		else
		{
			textToCursorSize = bodyElemState->font->computeTextSize(pwdStr);
			textToCursorSize.width *= textToCursor.size();
		}

		const f32 cursorWidth = bodyTextCaretElemState.width;
		const f32 cursorBorder = bodyTextCaretElemState.border;

		Rect cursorRect(
			clipRect.x + textToCursorSize.width - ctx->textInput.scrollOffset,
			clipRect.y + cursorBorder,
			cursorWidth,
			clipRect.height - cursorBorder * 2);

		if (ctx->textInput.selectionActive)
		{
			i32 startSel = ctx->textInput.selectionBegin, endSel = ctx->textInput.selectionEnd, tmpSel;

			if (startSel > endSel)
			{
				tmpSel = startSel;
				startSel = endSel;
				endSel = tmpSel;
			}

			if (endSel > ctx->textInput.text.size())
				endSel = ctx->textInput.text.size();
			if (startSel > ctx->textInput.text.size())
				startSel = ctx->textInput.text.size();

			FontTextSize selectedTextSize;
			FontTextSize textToSelectionStartSize;

			Utf32String selectedText = Utf32String(ctx->textInput.text.begin() + startSel, ctx->textInput.text.begin() + endSel);
			Utf32String textToSelectionStart = Utf32String(ctx->textInput.text.begin(), ctx->textInput.text.begin() + startSel);

			if (!password)
			{
				selectedTextSize = bodyElemState->font->computeTextSize(selectedText);
				textToSelectionStartSize = bodyElemState->font->computeTextSize(textToSelectionStart);
			}
			else
			{
				selectedTextSize = bodyElemState->font->computeTextSize(pwdStr);
				textToSelectionStartSize = selectedTextSize;
				selectedTextSize.width *= selectedText.size();
				textToSelectionStartSize.width *= textToSelectionStart.size();
			}

			Rect selRect(
				clipRect.x + textToSelectionStartSize.width - ctx->textInput.scrollOffset,
				clipRect.y,
				selectedTextSize.width,
				clipRect.height);

			// draw selection rect
			ctx->renderer.cmdSetColor(bodyTextSelectionElemState.color);
			ctx->renderer.cmdDrawFilledRectangle(selRect);
		}

		// draw cursor/caret
		if (!ctx->settings.textCaretBlinkEnable || (ctx->textInput.caretBlinkTimer >= 0 && ctx->textInput.caretBlinkTimer <= 1))
		{
			ctx->renderer.cmdSetColor(bodyTextCaretElemState.color);
			ctx->renderer.cmdDrawFilledRectangle(cursorRect);
		}
	}

	if (isEditingThis)
	{
		memset((char*)text, 0, maxLength);
		ctx->settings.services.utf32To8NoAlloc(ctx->textInput.text.data(), ctx->textInput.text.size(), text, maxLength);
	}

	if (password && defaultText != textToDraw)
	{
		u32 len = std::min(ctx->settings.services.utf8Length(textToDraw), maxHiddenCharLen);
		hiddenPwdText[0] = 0;

		for (i32 i = 0; i < len; i++)
		{
			strcat(hiddenPwdText, passwordChar);
		}

		textToDraw = hiddenPwdText;
	}

	if (textToDraw)
		isEmptyText = !strcmp(textToDraw, "");
	else
		isEmptyText = true;

	if (isEmptyText && defaultText)
	{
		textToDraw = (char*)defaultText;
		ctx->renderer.cmdSetColor(bodyTextDefaultElemState.textColor);
	}
	else
	{
		ctx->renderer.cmdSetColor(bodyElemState->textColor);
	}

	auto textRect = Rect(
		clipRect.x - (isEditingThis ? ctx->textInput.scrollOffset : 0),
		clipRect.y,
		clipRect.width,
		clipRect.height);

	// draw the actual text
	ctx->renderer.cmdDrawTextInBox(
		textToDraw,
		textRect,
		HAlignType::Left,
		VAlignType::Bottom, false, true);

	// draw clear image only when visible
	if (showClearImage)
	{
		// use hoveredState when clearFilterHovered is true, otherwise normalState.
		ThemeElement::State* state = nullptr;

		if (ctx->textInput.clearFilterHovered)
			state = &bodyTextFilterClearImageElem.hoveredState();
		else
			state = &bodyTextFilterClearImageElem.normalState();

		ctx->renderer.cmdSetColor(state->color);
		ctx->renderer.cmdDrawImage(state->image,
			Point(
				clipRect.right() - (state->border + padding.x) * ctx->scale - state->image->width * ctx->scale,
				clipRect.y + (clipRect.height - state->image->height * ctx->scale) / 2.0f),
			ctx->scale);
	}

	ctx->renderer.popClipRect();

	widgetSetFocusable();

	if (ctx->settings.textCaretBlinkSpeed > 0 && ctx->widget.focused)
	{
		// this will work even if deltaTime is always zero, caret wont blink ever
		// dt zero happens when UI is not drawn continuously
		ctx->textInput.caretBlinkTimer += ctx->settings.deltaTime * ctx->settings.textCaretBlinkSpeed;

		if (ctx->textInput.caretBlinkTimer > 2.0f)
		{
			ctx->textInput.caretBlinkTimer = 0;
		}
	}

	return ctx->textInput.textChanged;
}

}