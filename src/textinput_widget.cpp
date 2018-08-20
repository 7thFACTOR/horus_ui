#include "horus.h"
#include "types.h"
#include "ui_theme.h"
#include "renderer.h"
#include "text_cache.h"
#include "ui_font.h"
#include "ui_context.h"
#include "util.h"
#include <math.h>
#include <string.h>

namespace hui
{
bool textInput(
	Utf8String text,
	size_t maxLength,
	TextInputValueMode valueMode,
	Utf8String defaultText,
	Image icon)
{
	auto bodyElem = &ctx->theme->getElement(WidgetElementId::TextInputBody);
	auto bodyTextCaretElemState = ctx->theme->getElement(WidgetElementId::TextInputCaret).normalState();
	auto bodyTextSelectionElemState = ctx->theme->getElement(WidgetElementId::TextInputSelection).normalState();
	auto bodyTextDefaultElemState = ctx->theme->getElement(WidgetElementId::TextInputDefaultText).normalState();

	addWidgetItem(fmaxf(bodyElem->normalState().height * ctx->globalScale, bodyElem->normalState().font->getMetrics().height));

	if (!ctx->focusChanged)
		buttonBehavior();

	auto bodyElemState = &bodyElem->normalState();
	bool isEditingThis =
		ctx->currentWidgetId == ctx->textInput.widgetId
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

	bool editNow = false;

	if (ctx->event.type == InputEvent::Type::Key
		&& ctx->event.key.code == KeyCode::Enter
		&& ctx->event.key.down
		&& ctx->widget.focused)
	{
		if (!ctx->textInput.widgetId)
		{
			ctx->textInput.widgetId = ctx->currentWidgetId;
			isEditingThis = true;
			editNow = true;
			ctx->textInput.selectAllOnFocus = true;
		}
		else
		{
			ctx->textInput.widgetId = 0;
			isEditingThis = false;
		}
	}

	if (ctx->focusChanged
		&& ctx->currentWidgetId == ctx->widget.focusedWidgetId)
	{
		editNow = true;
		isEditingThis = 0 != ctx->textInput.widgetId;
		ctx->textInput.selectAllOnFocus = true;

		if (isEditingThis)
		{
			ctx->textInput.widgetId = ctx->currentWidgetId;
		}
	}

	if (ctx->widget.pressed
		&& ctx->currentWidgetId != ctx->textInput.widgetId)
	{
		ctx->textInput.widgetId = ctx->currentWidgetId;
		editNow = true;
		isEditingThis = true;
		ctx->textInput.selectAllOnFocus = true;
		ctx->textInput.firstMouseDown = true;
		ctx->widget.pressed = false;
		ctx->widget.focusedWidgetPressed = false;
	}

	if (editNow)
	{
		ctx->textInput.rect = ctx->widget.rect;
		ctx->textInput.clipRect = clipRect;
		ctx->textInput.maxTextLength = maxLength;
		ctx->textInput.selectionActive = false;
		ctx->textInput.valueType = valueMode;
		ctx->textInput.scrollOffset = 0;
		utf8ToUtf32(text, ctx->textInput.text);
		utf8ToUtf32(defaultText, ctx->textInput.defaultText);

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
		ctx->inputProvider->startTextInput(0, rc);
		bodyElemState = &bodyElem->getState(WidgetStateType::Focused);
		forceRepaint();
	}

	ctx->renderer->cmdSetColor(bodyElemState->color);
	ctx->renderer->cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->globalScale);
	ctx->renderer->cmdSetColor(bodyElemState->textColor);
	ctx->renderer->cmdSetFont(bodyElemState->font);
	ctx->renderer->pushClipRect(clipRect);

	if (isEditingThis)
	{
		int offs = ctx->textInput.caretPosition;

		if (ctx->textInput.caretPosition > ctx->textInput.text.size())
			offs = ctx->textInput.text.size() - 1;

		UnicodeString textToCursor = UnicodeString(
			ctx->textInput.text.begin(), ctx->textInput.text.begin() + offs);

		FontTextSize textToCursorSize = bodyElemState->font->computeTextSize(textToCursor);

		const f32 cursorWidth = bodyTextCaretElemState.width;
		const f32 cursorBorder = bodyTextCaretElemState.border;

		Rect cursorRect(
			clipRect.x + textToCursorSize.width - ctx->textInput.scrollOffset,
			clipRect.y + cursorBorder,
			cursorWidth,
			clipRect.height - cursorBorder * 2);

		if (ctx->textInput.selectionActive)
		{
			int startSel = ctx->textInput.selectionBegin, endSel = ctx->textInput.selectionEnd, tmpSel;

			if (startSel > endSel)
			{
				tmpSel = startSel;
				startSel = endSel;
				endSel = tmpSel;
			}

			UnicodeString selectedText = UnicodeString(ctx->textInput.text.begin() + startSel, ctx->textInput.text.begin() + endSel);
			UnicodeString textToSelectionStart = UnicodeString(ctx->textInput.text.begin(), ctx->textInput.text.begin() + startSel);
			FontTextSize selectedTextSize = bodyElemState->font->computeTextSize(selectedText);
			FontTextSize textToSelectionStartSize = bodyElemState->font->computeTextSize(textToSelectionStart);

			Rect selRect(
				clipRect.x + textToSelectionStartSize.width - ctx->textInput.scrollOffset,
				clipRect.y,
				selectedTextSize.width,
				clipRect.height);

			// draw selection rect
			ctx->renderer->cmdSetColor(bodyTextSelectionElemState.color);
			ctx->renderer->cmdDrawSolidColor(selRect);
		}

		// draw cursor
		ctx->renderer->cmdSetColor(bodyTextCaretElemState.color);
		ctx->renderer->cmdDrawSolidColor(cursorRect);
	}

	UnicodeString* textToDraw = nullptr;
	UnicodeString utf32text;
	bool isEmptyText = false;

	if (isEditingThis)
	{
		textToDraw = &ctx->textInput.text;
	}
	else
	{
		utf8ToUtf32(text, utf32text);
		textToDraw = &utf32text;
	}

	isEmptyText = textToDraw->empty();

	if (isEmptyText && defaultText)
	{
		textToDraw = ctx->textCache->getText(defaultText);
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

	utf32ToUtf8NoAlloc(*textToDraw, text, maxLength);

	// draw the actual text
	ctx->renderer->cmdDrawTextInBox(
		text,
		textRect,
		HAlignType::Left,
		VAlignType::Bottom);

	ctx->renderer->popClipRect();

	if (isEditingThis)
	{
		memset((char*)text, 0, maxLength);
		utf32ToUtf8NoAlloc(ctx->textInput.text, text, maxLength);
	}

	setAsFocusable();
	ctx->currentWidgetId++;

	return ctx->textInput.textChanged;
}

}
