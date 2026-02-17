#include <string.h>
#include <algorithm>
#include "context.h"
#include "theme.h"
#include "renderer.h"
#include "font.h"
#include <cmath>
#include "util.h"

namespace hui
{
bool multilineTextInput(
	const char* id,
	char* text,
	u32 maxLength,
	u32 visibleLines,
	MultilineTextInputFlags flags)
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

	// calculate height based on visible line count
	Font* font = bodyElem->normalState().font;
	f32 lineHeight = font ? font->getMetrics().height : 20.0f;
	f32 border = bodyElem->normalState().border;
	f32 totalHeight = visibleLines * lineHeight + (padding.y + border) * 2.0f;

	ctx->id = genId(id);
	addWidget(totalHeight);

	buttonBehavior();

	// pre-calculate scroll ID for focus checks
	std::string scrollIdName = std::string(id) + ".scroller";
	WidgetId scrollId = genId(scrollIdName.c_str());
	if (state.id == ctx->id)
		state.scrollId = scrollId;

	if (ctx->focusChanged && ctx->id != ctx->widget.focusedId && ctx->widget.focusedId != state.scrollId)
		ctx->widget.changeEnded = true;

	auto bodyElemState = &bodyElem->normalState();
	bool isEditingThis =
		(ctx->id == state.id || state.id == ctx->widget.focusedId)
		&& (ctx->widget.focused || ctx->widget.focusedId == state.scrollId)
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

	// handle Enter key differently - don't end editing
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
		}
	}

	if (ctx->widget.pressed && ctx->id != state.id)
	{
		state.id = ctx->id;
		state.editNow = true;
		isEditingThis = true;
		state.selectAllOnFocus = true;
		state.firstMouseDown = true;
	}

	if (state.editNow)
	{
		state.rect = ctx->widget.rect;
		state.clipRect = clipRect;
		state.maxTextLength = maxLength;
		state.selectionActive = false;
		state.flags = flags;



		// parse existing text into lines
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

		if (state.selectAllOnFocus && has(flags, MultilineTextInputFlags::AutoSelectAll))
			state.selectAll();
		else
		{
			state.selectionActive = false;
			state.getCharIndexAtPoint(ctx->mousePosition);
			state.mouseDown = true;
			state.mouseDownSelectionStartLine = state.currentLine;
			state.mouseDownSelectionStartColumn = state.caretColumn;
			// Reset selection start/end to current cursor position to ensure drag selection starts correctly
			state.selectionStartLine = state.selectionEndLine = state.currentLine;
			state.selectionStartColumn = state.selectionEndColumn = state.caretColumn;
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


	// calculate sidebar width based on line count
	f32 sidebarWidth = 0.0f;
	Rect originalClipRect = clipRect;

	if (has(flags, MultilineTextInputFlags::LineNumbers))
	{
		auto digitCount = [](int n) {
			if (n == 0) return 1;
			return (int)(std::floor(std::log10(std::abs(n))) + 1);
		};

		u32 digits = digitCount(state.lines.size());
		f32 charWidth = font->computeTextSize("0", 1).width;
		sidebarWidth = digits * charWidth + 10.0f; // Padding

		// adjust clip rect to exclude sidebar
		f32 sidebarTextGap = 5.0f * ctx->scale;
		clipRect.x += sidebarWidth + sidebarTextGap;
		clipRect.width -= (sidebarWidth + sidebarTextGap);
	}

	// update state rects every frame so ensureCaretVisible works correctly
	if (state.id == ctx->id)
	{
		state.rect = ctx->widget.rect;
		state.clipRect = clipRect;
	}

	// state.scrollId is already calculated above


	// process Input (Typing, Navigation)
	// input processing is handled in beginFrame() for the active widget
	// so we don't need to call it manually here to avoid double processing.
	if (isEditingThis)
	{
		// state.processEvent(ctx->event);
	}

	if (isEditingThis)
	{
		// write back to text buffer
		Utf32String fullText;
		for (size_t i = 0; i < state.lines.size(); i++)
		{
			fullText.insert(fullText.end(), state.lines[i].begin(), state.lines[i].end());
			if (i < state.lines.size() - 1)
				fullText.push_back('\n');
		}

		ctx->settings.services.utf32To8NoAlloc(fullText.data(), fullText.size(), text, maxLength);
	}

	// draw background
	ctx->renderer.cmdSetColor(bodyElemState->color);
	ctx->renderer.cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->scale);

	// calculate total content height early for scrollbar detection
	f32 totalContentHeight = state.lines.size() * lineHeight;

	// calculate maxLineWidth early for scrollbar detection
	f32 maxLineWidth = 0;
	{
		Font* calcFont = bodyElemState->font;
		if (calcFont)
		{
			for (const auto& line : state.lines)
			{
				FontTextSize size = calcFont->computeTextSize(line.data(), (u32)line.size());
				if (size.width > maxLineWidth)
					maxLineWidth = size.width;
			}
		}
	}

	f32 scrollAreaV = clipRect.height;
	f32 scrollAreaH = clipRect.width;
	bool hasVerticalScrollbar = false;
	bool hasHorizontalScrollbar = false;

	auto& sbV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();
	auto& sbH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();

	// check if horizontal scrollbar is needed first (simplified logic from ScrollView)
	if ((maxLineWidth + 10.0f) > scrollAreaH)
	{
		hasHorizontalScrollbar = true;
		scrollAreaV -= sbH.height * ctx->scale;
	}

	// check if vertical scrollbar is needed
	if (totalContentHeight > scrollAreaV)
	{
		hasVerticalScrollbar = true;
		scrollAreaH -= sbV.width * ctx->scale;
	}

	// re-check horizontal with reduced width
	if (!hasHorizontalScrollbar && (maxLineWidth + 10.0f) > scrollAreaH)
	{
		hasHorizontalScrollbar = true;
		scrollAreaV -= sbH.height * ctx->scale;

		// re-check vertical with reduced height
		if (!hasVerticalScrollbar && totalContentHeight > scrollAreaV)
		{
			hasVerticalScrollbar = true;
		}
	}

	// draw sidebar background
	Rect sidebarRect = originalClipRect;

	if (has(flags, MultilineTextInputFlags::LineNumbers))
	{
		ctx->renderer.pushClipRect(originalClipRect);

		sidebarRect.width = sidebarWidth;
		ctx->renderer.cmdSetColor(Color::darkGray); // Gray
		ctx->renderer.cmdDrawFilledRectangle(sidebarRect);
	}
	else
	{
		// if no sidebar, push clip rect anyway to match popping later and for safety
		ctx->renderer.pushClipRect(originalClipRect);
	}

	ctx->renderer.cmdSetColor(bodyElemState->textColor);
	ctx->renderer.cmdSetFont(bodyElemState->font);

	// gets current scroll state to use for sidebar culling
	auto& scrollStateEarly = ctx->scrollViewState[state.scrollId];
	f32 currentScrollYEarly = scrollStateEarly.scrollOffset.y;

	i32 startLine = (i32)(currentScrollYEarly / lineHeight);
	i32 endLine = startLine + visibleLines + 2;

	// draw line highlights and numbers
	for (i32 i = startLine; i < endLine && i < state.lines.size(); i++)
	{
		f32 yPos = clipRect.y + i * lineHeight - currentScrollYEarly;

		// use originalClipRect for visibility check because clipRect is indented
		if (yPos + lineHeight < originalClipRect.y || yPos > originalClipRect.bottom())
			continue;

		// current line highlighting
		if (i == state.currentLine && isEditingThis)
		{
			ctx->renderer.cmdSetColor(Color::black);

			// line number highlight (Dark Cyan)
			if (has(flags, MultilineTextInputFlags::LineNumbers))
			{
				Rect lineNumRect = sidebarRect;
				lineNumRect.y = yPos;
				lineNumRect.height = lineHeight;
				ctx->renderer.cmdDrawFilledRectangle(lineNumRect);
			}

			// text highlight (Dark Green)
			if (has(flags, MultilineTextInputFlags::HighlightCurrentLine))
			{
				Rect lineTextRect = clipRect;
				lineTextRect.y = yPos;
				lineTextRect.height = lineHeight;

				// trim highlight width to avoid drawing under vertical scrollbar
				if (hasVerticalScrollbar)
				{
					lineTextRect.width -= sbV.width * ctx->scale;
				}

				// clip the highlight to the text area (using clipRect)
				ctx->renderer.cmdDrawFilledRectangle(lineTextRect);
			}
		}

		// draw line number
		if (has(flags, MultilineTextInputFlags::LineNumbers))
		{
			char numStr[32];
			sprintf(numStr, "%d", i + 1);
			Rect numRect = sidebarRect;
			numRect.y = yPos;
			numRect.height = lineHeight;
			numRect.width -= 5.0f; // Padding

			ctx->renderer.cmdSetColor(Color::white);
			ctx->renderer.cmdDrawTextInBox(numStr, numRect, HAlignType::Right, VAlignType::Center, false, true);
		}
	}

	ctx->renderer.popClipRect();

	// set cursor to IBeam only if not hovering over scrollbars
	if (ctx->widget.hovered)
	{
		bool overScrollbar = false;

		auto& sbV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();
		auto& sbH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();

		f32 scrollAreaV = clipRect.height;
		f32 availableWidth = clipRect.width;

		// horizontal scrollbar logic
		bool hasHorizontalScrollbar = false;
		// logic from ScrollView: (virtualSize.x > 0 && virtualSize.x > availableWidth) || scrollContentH > availableWidth
		// here virtualSize.x is maxLineWidth. scrollContentH is roughly maxLineWidth if we trust Multiline input structure.
		if ((maxLineWidth + 10.0f) > availableWidth)
		{
			hasHorizontalScrollbar = true;
			scrollAreaV -= sbH.height * ctx->scale;
		}

		// vertical scrollbar logic
		bool hasVerticalScrollbar = false;
		if (totalContentHeight > scrollAreaV)
		{
			hasVerticalScrollbar = true;
			availableWidth -= sbV.width * ctx->scale;
		}

		// re-evaluate Horizontal with reduced available width if Vertical is present
		if (!hasHorizontalScrollbar && (maxLineWidth + 10.0f) > availableWidth)
		{
			hasHorizontalScrollbar = true;
			scrollAreaV -= sbH.height * ctx->scale;
			// re-evaluate Vertical with reduced height (optional, but consistent)
			if (!hasVerticalScrollbar && totalContentHeight > scrollAreaV)
				hasVerticalScrollbar = true;
		}

		if (hasVerticalScrollbar)
		{
			// vertical scrollbar rect (right side of clipRect)
			Rect vRect = clipRect;
			vRect.x = vRect.right() - sbV.width * ctx->scale;
			vRect.width = sbV.width * ctx->scale;
			// be careful: ScrollView might deduce height if H-scroll is present
			if (hasHorizontalScrollbar)
				vRect.height -= sbH.height * ctx->scale;

			if (vRect.contains(ctx->mousePosition))
				overScrollbar = true;
		}

		if (hasHorizontalScrollbar)
		{
			// horizontal scrollbar rect (bottom of clipRect)
			Rect hRect = clipRect;
			hRect.y = hRect.bottom() - sbH.height * ctx->scale;
			hRect.height = sbH.height * ctx->scale;
			// be careful: if V-scroll is present, full width might be reduced?
			// in ScrollView:
			// f32 scrollBarWidthFull = rectNoBorders.width;
			// if (scrollContentSizeV > ...) scrollBarWidthFull -= ...;
			// Rect rectScrollBarH = { ..., scrollBarWidthFull, ... }
			// So yes, H scrollbar doesn't extend under V scrollbar usually.
			if (hasVerticalScrollbar)
				hRect.width -= sbV.width * ctx->scale;

			if (hRect.contains(ctx->mousePosition))
				overScrollbar = true;
		}

		if (overScrollbar)
			setMouseCursor(MouseCursorType::Arrow);
		else
			setMouseCursor(MouseCursorType::IBeam);
	}

	pushLayout();

	// begin ScrollView (it handles layout, scrollbars, and inputs)
	// we must reset position to inside the wrapper because addWidget() moved it to the bottom
	Point wrapperEndPos = ctx->position;
	ctx->position = { clipRect.x, clipRect.y };

	// ensure we pass a unique ID for the scroll view distinct from the wrapper if needed,
	// or append string to ID.
	// (scrollIdName was already generated above)

	// fix 2: Constrain ScrollView width to the clipRect width (since we indented position)
	f32 savedLayoutWidth = ctx->layout.width;
	ctx->layout.width = clipRect.width;

	// scale height down because beginScrollView scales it up again (double-scaling fix)
	f32 scrollViewHeight = clipRect.height;
	if (ctx->settings.scaleScrollViewHeight)
		scrollViewHeight /= ctx->scale;

	// use existing scroll offset if available to persist scrolling
	Point initialScroll = {0, 0};
	if (state.scrollId != 0)
	{
		auto it = ctx->scrollViewState.find(state.scrollId);
		if (it != ctx->scrollViewState.end())
			initialScroll = it->second.scrollOffset;
	}

	beginScrollView(scrollIdName.c_str(), scrollViewHeight, initialScroll, { maxLineWidth + 10.0f, totalContentHeight }, ScrollViewFlags::NoBorder | ScrollViewFlags::NoPadding);


	ctx->layout.width = savedLayoutWidth; // Restore layout width immediately (beginScrollView captured it)

	// update state clip rect to the inner clip rect (excluding scrollbars)
	// this prevents drawing over scrollbars and ensures clicks on scrollbars aren't handled as text input
	Rect innerClipRect = ctx->renderer.getClipRect();
	if (state.id == ctx->id)
	{
		state.clipRect = innerClipRect;
		state.rect = ctx->widget.rect;
	}

	// use inner clip rect for local drawing logic
	clipRect = innerClipRect;

	// state.scrollId is already set above

	// gets current scroll state to use for culling
	auto& scrollState = ctx->scrollViewState[state.scrollId];
	f32 currentScrollY = scrollState.scrollOffset.y;
	f32 currentScrollX = scrollState.scrollOffset.x;

	ctx->renderer.cmdSetColor(bodyElemState->textColor);

	// draw visible lines
	i32 firstLine = (i32)(currentScrollY / lineHeight);
	i32 lastLine = firstLine + visibleLines + 2; // +buffer

	// draw selection
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

			// if selection width is 0 and we are selecting a newline (endCol == size), give it a width of a space
			// to visualize the newline selection
			if (selRect.width <= 0.001f && colEnd == state.lines[line].size())
			{
				static f32 spaceWidth = 0.0f;
				if (spaceWidth == 0.0f)
					spaceWidth = font->computeTextSize(" ", 1).width;
				selRect.width = spaceWidth;
			}

			ctx->renderer.cmdSetColor(bodyTextSelectionElemState.color);
			ctx->renderer.cmdDrawFilledRectangle(selRect);
		}
	}

	// draw caret
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

		// ensure caret is drawn within clip rect (ScrollView handles clipping too, but we draw explicitly)
		if (cursorRect.y + cursorRect.height > clipRect.y && cursorRect.y < clipRect.bottom())
		{
			ctx->renderer.cmdSetColor(bodyTextCaretElemState.color);
			ctx->renderer.cmdDrawFilledRectangle(cursorRect);
		}
	}

	// draw text
	for (i32 i = firstLine; i < lastLine && i < state.lines.size(); i++)
	{
		f32 yPos = clipRect.y + i * lineHeight - currentScrollY;

		// double check visibility
		if (yPos + lineHeight < clipRect.y || yPos > clipRect.bottom())
			continue;

		Rect textRect;
		textRect.x = clipRect.x - currentScrollX;
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

	// advance layout position so ScrollView knows the content height
	ctx->position.y += totalContentHeight;

	endScrollView();
	ctx->position = wrapperEndPos; // Restore layout position
	// fix 3: Removed unbalanced popClipRect() here (beginScrollView handles its own push/pop)

	setFocusable();

	if (ctx->settings.textCaretBlinkSpeed > 0 && ctx->widget.focused)
	{
		state.caretBlinkTimer += ctx->settings.deltaTime * ctx->settings.textCaretBlinkSpeed;

		if (state.caretBlinkTimer > 2.0f)
			state.caretBlinkTimer = 0;
	}

	popLayout();

	return state.textChanged;
}

}
