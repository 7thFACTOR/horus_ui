#include <string.h>
#include <algorithm>
#include "context.h"
#include "theme.h"
#include "renderer.h"
#include "font.h"
#include "util.h"

namespace hui
{
bool multilineTextInput(
	const char* id,
	char* text,
	u32 maxLength,
	u32 visibleLines,
	TextInputFlags flags)
{
	auto bodyElem = &ctx->theme->getElement(WidgetElementId::TextInputBody);
	auto& bodyTextCaretElemState = ctx->theme->getElement(WidgetElementId::TextInputCaret).normalState();
	auto& bodyTextSelectionElemState = ctx->theme->getElement(WidgetElementId::TextInputSelection).normalState();
	auto& padding = getWidgetPadding();

	auto& state = ctx->multilineTextInput;
	state.visibleLineCount = visibleLines;

	if (!ctx->widget.hasNextWidth)
	{
		ctx->widget.customWidth = ctx->layout.width / ctx->scale;
		ctx->widget.hasCustomWidth = true;
	}

	// Calculate height based on visible line count
	Font* font = bodyElem->normalState().font;
	f32 lineHeight = font ? font->getMetrics().height : 20.0f;
	f32 totalHeight = visibleLines * lineHeight + (padding.y * 2.0f);

	ctx->id = genId(id);
	addWidget(totalHeight);

	buttonBehavior();

	if (ctx->focusChanged && ctx->id != ctx->widget.focusedId)
		ctx->widget.changeEnded = true;

	auto bodyElemState = &bodyElem->normalState();
	bool isEditingThis =
		ctx->id == state.id
		&& ctx->widget.focused
		&& ctx->isActiveLayer();

	state.themeElement = bodyElem;

	if (ctx->widget.focused)
		bodyElemState = &bodyElem->getState(WidgetStateType::Focused);

	auto clipRect = Rect(
		ctx->widget.rect.x + (bodyElemState->border + padding.x) * ctx->scale,
		ctx->widget.rect.y + (bodyElemState->border + padding.y) * ctx->scale,
		ctx->widget.rect.width - (bodyElemState->border + padding.x) * 2.0f * ctx->scale,
		ctx->widget.rect.height - (bodyElemState->border + padding.y) * 2.0f * ctx->scale);

	state.editNow = false;

	// Handle Enter key differently - don't end editing
	if (ctx->event.type == InputEvent::Type::Key
		&& ctx->event.key.down
		&& ctx->widget.focused)
	{
		if (ctx->event.key.code == KeyCode::Esc)
		{
			state.id = 0;
			state.editNow = false;
			isEditingThis = false;
			ctx->widget.focusedId = 0;
			ctx->widget.changeEnded = true;
			ctx->widget.pressed = false;
		}
	}

	if (ctx->focusChanged && ctx->id == ctx->widget.focusedId)
	{
		state.editNow = true;
		isEditingThis = 0 != state.id;
		state.selectAllOnFocus = true;

		if (isEditingThis)
		{
			state.id = ctx->id;
			// Reset scroll offset when starting to edit
			std::string scrollIdName = std::string(id) + ".scroller";
			WidgetId scrollerId = genId(scrollIdName.c_str());
			ctx->scrollViewState[scrollerId].scrollOffset = {0, 0};
		}
	}

	if (ctx->widget.pressed && ctx->id != state.id)
	{
		state.id = ctx->id;
		state.editNow = true;
		isEditingThis = true;
		state.selectAllOnFocus = true;
		state.firstMouseDown = true;
		
		// Reset scroll offset when starting to edit
		std::string scrollIdName = std::string(id) + ".scroller";
		WidgetId scrollerId = genId(scrollIdName.c_str());
		ctx->scrollViewState[scrollerId].scrollOffset = {0, 0};
	}

	if (state.editNow)
	{
		state.rect = ctx->widget.rect;
		state.clipRect = clipRect;
		state.maxTextLength = maxLength;
		state.selectionActive = false;
		state.flags = flags;
		


		// Parse existing text into lines
		Utf32String fullText;
		ctx->settings.services.utf8To32(text, fullText);

		state.lines.clear();
		Utf32String currentLine;
		for (u32 ch : fullText)
		{
			if (ch == '\n')
			{
				state.lines.push_back(currentLine);
				currentLine.clear();
			}
			else
			{
				currentLine.push_back(ch);
			}
		}
		state.lines.push_back(currentLine);
		if (state.lines.empty())
			state.lines.push_back(Utf32String());

		if (state.selectAllOnFocus && has(flags, TextInputFlags::AutoSelectAll))
			state.selectAll();
		else
		{
			state.selectionActive = false;
			state.getCharIndexAtPoint(ctx->mousePosition);
			state.mouseDown = true;
			state.mouseDownSelectionStartLine = state.currentLine;
			state.mouseDownSelectionStartColumn = state.caretColumn;
			state.mouseMoved = false;
			state.selectingWithMouse = false;
			state.ensureCaretVisible();
			setWindowCapture();
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

	if (ctx->widget.hovered)
		setMouseCursor(MouseCursorType::IBeam);

	// Update state rects every frame so ensureCaretVisible works correctly
	state.rect = ctx->widget.rect;
	state.clipRect = clipRect;
	
	// Pre-calculate scroll ID so that processEvent -> ensureCaretVisible can update the correct scroll state
	std::string scrollIdName = std::string(id) + ".scroller";
	state.scrollId = genId(scrollIdName.c_str());

	// Process Input (Typing, Navigation)
	if (isEditingThis)
	{
		state.processEvent(ctx->event);
	}

	// Draw background
	ctx->renderer.cmdSetColor(bodyElemState->color);
	ctx->renderer.cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->scale);
	ctx->renderer.cmdSetColor(bodyElemState->textColor);
	ctx->renderer.cmdSetFont(bodyElemState->font);
	// Calculate content size for ScrollView
	f32 maxLineWidth = 0;
	for (const auto& line : state.lines)
	{
		FontTextSize size = font->computeTextSize(line.data(), (u32)line.size());
		if (size.width > maxLineWidth)
			maxLineWidth = size.width;
	}
	f32 totalContentHeight = state.lines.size() * lineHeight;

	// Begin ScrollView (it handles layout, scrollbars, and inputs)
	// We must reset position to inside the wrapper because addWidget() moved it to the bottom
	Point wrapperEndPos = ctx->position;
	ctx->position = { clipRect.x, clipRect.y };
	
	// Ensure we pass a unique ID for the scroll view distinct from the wrapper if needed, 
	// or append string to ID.
	// (scrollIdName was already generated above)
	
	// Fix 2: Constrain ScrollView width to the clipRect width (since we indented position)
	f32 savedLayoutWidth = ctx->layout.width;
	ctx->layout.width = clipRect.width;

	// Scale height down because beginScrollView scales it up again (double-scaling fix)
	f32 scrollViewHeight = clipRect.height;
	if (ctx->settings.scaleScrollViewHeight)
		scrollViewHeight /= ctx->scale;

	// Use existing scroll offset if available to persist scrolling
	Point initialScroll = {0, 0};
	if (state.scrollId != 0)
	{
		auto it = ctx->scrollViewState.find(state.scrollId);
		if (it != ctx->scrollViewState.end())
			initialScroll = it->second.scrollOffset;
	}

	beginScrollView(scrollIdName.c_str(), scrollViewHeight, initialScroll, { maxLineWidth + 10.0f, totalContentHeight }, ScrollViewFlags::NoBorder | ScrollViewFlags::NoPadding);
	
	ctx->layout.width = savedLayoutWidth; // Restore layout width immediately (beginScrollView captured it)

	// state.scrollId is already set above

	// Gets current scroll state to use for culling
	auto& scrollState = ctx->scrollViewState[state.scrollId];
	f32 currentScrollY = scrollState.scrollOffset.y;
	f32 currentScrollX = scrollState.scrollOffset.x;

	ctx->renderer.cmdSetColor(bodyElemState->textColor);

	// Draw visible lines
	i32 firstLine = (i32)(currentScrollY / lineHeight);
	i32 lastLine = firstLine + visibleLines + 2; // +buffer

	// Draw selection
	if (isEditingThis && state.selectionActive)
	{
		i32 startLine = state.selectionStartLine;
		i32 startCol = state.selectionStartColumn;
		i32 endLine = state.selectionEndLine;
		i32 endCol = state.selectionEndColumn;

		if (startLine > endLine || (startLine == endLine && startCol > endCol))
		{
			std::swap(startLine, endLine);
			std::swap(startCol, endCol);
		}

		for (i32 line = startLine; line <= endLine && line < state.lines.size(); line++)
		{
			f32 yPos = clipRect.y + line * lineHeight - currentScrollY;

			if (yPos + lineHeight < clipRect.y || yPos > clipRect.bottom())
				continue;

			i32 colStart = (line == startLine) ? startCol : 0;
			i32 colEnd = (line == endLine) ? endCol : state.lines[line].size();

			Utf32String textToStart(state.lines[line].begin(), state.lines[line].begin() + colStart);
			Utf32String selectedText(state.lines[line].begin() + colStart, state.lines[line].begin() + colEnd);

			FontTextSize toStartSize = font->computeTextSize(textToStart.data(), (u32)textToStart.size());
			FontTextSize selectedSize = font->computeTextSize(selectedText.data(), (u32)selectedText.size());

			Rect selRect;
			selRect.x = clipRect.x + toStartSize.width - currentScrollX;
			selRect.y = yPos;
			selRect.width = selectedSize.width;
			selRect.height = lineHeight;

			ctx->renderer.cmdSetColor(bodyTextSelectionElemState.color);
			ctx->renderer.cmdDrawFilledRectangle(selRect);
		}
	}

	// Draw caret
	if (isEditingThis && (!ctx->settings.textCaretBlinkEnable || (state.caretBlinkTimer >= 0 && state.caretBlinkTimer <= 1)))
	{
		Point caretPos = state.getCaretScreenPosition();
		const f32 cursorWidth = bodyTextCaretElemState.width;
		const f32 cursorBorder = bodyTextCaretElemState.border;

		Rect cursorRect;
		cursorRect.x = caretPos.x;
		cursorRect.y = caretPos.y + cursorBorder;
		cursorRect.width = cursorWidth;
		cursorRect.height = lineHeight - cursorBorder * 2;
		
		// Ensure caret is drawn within clip rect (ScrollView handles clipping too, but we draw explicitly)
		if (cursorRect.y + cursorRect.height > clipRect.y && cursorRect.y < clipRect.bottom())
		{
			ctx->renderer.cmdSetColor(bodyTextCaretElemState.color);
			ctx->renderer.cmdDrawFilledRectangle(cursorRect);
		}
	}

	// Draw text
	for (i32 i = firstLine; i < lastLine && i < state.lines.size(); i++)
	{
		f32 yPos = clipRect.y + i * lineHeight - currentScrollY;

		// Double check visibility
		if (yPos + lineHeight < clipRect.y || yPos > clipRect.bottom())
			continue;

		Rect textRect;
		textRect.x = clipRect.x - (isEditingThis ? currentScrollX : 0);
		textRect.y = yPos;
		textRect.width = clipRect.width; 
		textRect.height = lineHeight;

		ctx->renderer.cmdSetColor(bodyElemState->textColor);

		char* lineText = nullptr;
		ctx->settings.services.utf32To8(state.lines[i], &lineText);

		if (lineText && lineText[0])
		{
			ctx->renderer.cmdDrawTextInBox(
				lineText,
				textRect,
				HAlignType::Left,
				VAlignType::Bottom, false, true);
		}

		if (lineText)
			delete[] lineText;
	}

	// Advance layout position so ScrollView knows the content height
	ctx->position.y += totalContentHeight;

	endScrollView();
	ctx->position = wrapperEndPos; // Restore layout position
	// Fix 3: Removed unbalanced popClipRect() here (beginScrollView handles its own push/pop)

	setFocusable();

	if (ctx->settings.textCaretBlinkSpeed > 0 && ctx->widget.focused)
	{
		state.caretBlinkTimer += ctx->settings.deltaTime * ctx->settings.textCaretBlinkSpeed;

		if (state.caretBlinkTimer > 2.0f)
			state.caretBlinkTimer = 0;
	}

	return state.textChanged;
}

}
