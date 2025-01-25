#include <string.h>
#include <algorithm>
#include "context.h"
#include "theme.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool textInput(
	char* text,
	u32 maxLength,
	TextInputValueMode valueMode,
	const char* defaultText,
	HImage icon,
	bool password,
	const char* passwordChar)
{
	auto bodyElem = &ctx->theme->getElement(WidgetElementId::TextInputBody);
	auto& bodyTextCaretElemState = ctx->theme->getElement(WidgetElementId::TextInputCaret).normalState();
	auto& bodyTextSelectionElemState = ctx->theme->getElement(WidgetElementId::TextInputSelection).normalState();
	auto& bodyTextDefaultElemState = ctx->theme->getElement(WidgetElementId::TextInputDefaultText).normalState();

	// use ptr as id
	ctx->id = genId((void*)text);
	addWidget(fmaxf(bodyElem->normalState().height * ctx->scale, bodyElem->normalState().font->getMetrics().height));

	if (!ctx->focusChanged)
		buttonBehavior();

	if (ctx->focusChanged
		&& ctx->id != ctx->widget.focusedId)
	{
		ctx->widget.changeEnded = true;
	}

	auto bodyElemState = &bodyElem->normalState();
	bool isEditingThis =
		ctx->id == ctx->textInput.id
		&& ctx->widget.focused
		&& ctx->isActiveLayer();

	ctx->textInput.themeElement = bodyElem;

	if (ctx->widget.focused)
	{
		bodyElemState = &bodyElem->getState(WidgetStateType::Focused);
	}

	auto clipRect = Rect(
		ctx->widget.rect.x + bodyElemState->border,
		ctx->widget.rect.y + bodyElemState->border,
		ctx->widget.rect.width - bodyElemState->border * 2,
		ctx->widget.rect.height - bodyElemState->border * 2);

	const size_t maxHiddenCharLen = 1024;
	static char hiddenPwdText[maxHiddenCharLen] = "";
	bool isEmptyText = false;
	char* textToDraw = (char*)text;
	Utf32String pwdStr;

	HORUS_UTF->utf8To32(passwordChar, pwdStr);

	ctx->textInput.editNow = false;
	ctx->textInput.password = password;
	ctx->textInput.passwordCharUnicode = pwdStr;

	if (ctx->event.type == InputEvent::Type::Key
		&& ctx->event.key.code == KeyCode::Enter
		&& ctx->event.key.down
		&& ctx->widget.focused)
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
		ctx->widget.pressed = false;
		ctx->widget.focusedAndPressed = false;
	}

	if (ctx->textInput.editNow)
	{
		ctx->textInput.rect = ctx->widget.rect;
		ctx->textInput.clipRect = clipRect;
		ctx->textInput.maxTextLength = maxLength;
		ctx->textInput.selectionActive = false;
		ctx->textInput.valueType = valueMode;
		ctx->textInput.scrollOffset = 0;
		HORUS_UTF->utf8To32(text, ctx->textInput.text);
		HORUS_UTF->utf8To32(defaultText, ctx->textInput.defaultText);

		if (ctx->textInput.selectAllOnFocus)
		{
			ctx->textInput.selectAll();
		}

		// this must be called to handle the event in the text input ways
		// otherwise it needs a second click to do stuff for the edit box
		ctx->textInput.processEvent(ctx->event);

		Rect rc;

		rc.x = ctx->widget.rect.x;
		rc.y = ctx->widget.rect.y;
		rc.width = ctx->widget.rect.width;
		rc.height = ctx->widget.rect.height;
		HORUS_INPUT->startTextInput(ctx->lastHoveredNativeWindow, rc);
		bodyElemState = &bodyElem->getState(WidgetStateType::Focused);
		forceRepaint();
	}

	if (ctx->widget.hovered)
	{
		setMouseCursor(MouseCursorType::IBeam);
	}

	ctx->renderer->cmdSetColor(bodyElemState->color);
	ctx->renderer->cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->scale);
	ctx->renderer->cmdSetColor(bodyElemState->textColor);
	ctx->renderer->cmdSetFont(bodyElemState->font);
	ctx->renderer->pushClipRect(clipRect);

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
			ctx->renderer->cmdSetColor(bodyTextSelectionElemState.color);
			ctx->renderer->cmdDrawSolidRectangle(selRect);
		}

		// draw cursor/caret	
		if (!ctx->settings.textCaretBlinkEnable || (ctx->textInput.caretBlinkTimer >= 0 && ctx->textInput.caretBlinkTimer <= 1))
		{
			ctx->renderer->cmdSetColor(bodyTextCaretElemState.color);
			ctx->renderer->cmdDrawSolidRectangle(cursorRect);
		}
	}

	if (isEditingThis)
	{
		memset((char*)text, 0, maxLength);
		HORUS_UTF->utf32To8NoAlloc(ctx->textInput.text, text, maxLength);
	}

	if (password && defaultText != textToDraw)
	{
		u32 len = std::min(HORUS_UTF->utf8Length(textToDraw), maxHiddenCharLen);
		hiddenPwdText[0] = 0;

		for (i32 i = 0; i < len; i++)
		{
			strcat(hiddenPwdText, passwordChar);
		}

		textToDraw = hiddenPwdText;
	}

	isEmptyText = !strcmp(textToDraw, "");

	if (isEmptyText && defaultText)
	{
		textToDraw = (char*)defaultText;
		ctx->renderer->cmdSetColor(bodyTextDefaultElemState.color);
	}
	else
	{
		ctx->renderer->cmdSetColor(bodyElemState->color);
	}

	auto textRect = Rect(
		clipRect.x - (isEditingThis ? ctx->textInput.scrollOffset : 0),
		clipRect.y,
		clipRect.width,
		clipRect.height);

	// draw the actual text
	ctx->renderer->cmdDrawTextInBox(
		textToDraw,
		textRect,
		HAlignType::Left,
		VAlignType::Bottom);

	ctx->renderer->popClipRect();

	setFocusable();

	if (ctx->settings.textCaretBlinkDelay > 0)
	{
		// this will work even if deltaTime is always zero, caret wont blink ever
		// dt zero happens when UI is not drawn continuously
		ctx->textInput.caretBlinkTimer += ctx->deltaTime * ctx->settings.textCaretBlinkDelay;

		if (ctx->textInput.caretBlinkTimer > 2.0f)
		{
			ctx->textInput.caretBlinkTimer = 0;
		}
	}
	
	popId();

	return ctx->textInput.textChanged;
}

}