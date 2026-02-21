#include <string.h>
#include <algorithm>
#include "context.h"
#include "theme.h"
#include "renderer.h"
#include "font.h"
#include <cmath>
#include "util.h"
#include <chrono>

namespace hui
{
bool multilineTextInput(
	const char* id,
	char* text,
	u32 maxLength,
	u32 visibleLines,
	MultilineTextInputFlags flags,
	const KeywordInfo* keywords,
	u32 keywordCount,
	const RangeHighlight* rangeHighlights,
	u32 rangeHighlightCount)
{
	auto bodyElem = &ctx->theme->getElement(WidgetElementId::MultilineTextInputBody);
	auto& lineNumbersElem = ctx->theme->getElement(WidgetElementId::MultilineTextInputLineNumbers);
	auto& bodyTextCaretElemState = ctx->theme->getElement(WidgetElementId::TextInputCaret).normalState();
	auto& bodyTextSelectionElemState = ctx->theme->getElement(WidgetElementId::TextInputSelection).normalState();
	auto& currentLineHighlightElemState = ctx->theme->getElement(WidgetElementId::MultilineTextInputCurrentLineHighlight).normalState();
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

		printf("multilineTextInput: full re-parse/edit triggered (ID: %s)\n", id);



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

		state.totalTextLength = 0;
		for (const auto& line : state.lines)
			state.totalTextLength += line.size();
		if (state.lines.size() > 1)
			state.totalTextLength += state.lines.size() - 1; // newlines

		if (state.selectAllOnFocus && has(flags, MultilineTextInputFlags::AutoSelectAll))
			state.selectAll();
		else
		{
			state.selectionActive = false;
			state.getCharIndexAtPoint(ctx->mousePosition);
			state.mouseDown = true;
			state.mouseDownSelectionStartLine = state.currentLine;
			state.mouseDownSelectionStartColumn = state.caretColumn;
			// reset selection start/end to current cursor position to ensure drag selection starts correctly
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
		state.editNow = false;
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
		f32 charWidth = lineNumbersElem.normalState().font->computeTextSize("0", 1).width;
		sidebarWidth = digits * charWidth + 10.0f; // padding

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

	state.updateSyntaxHighlighting(rangeHighlights, rangeHighlightCount, keywords, keywordCount);

	// update visual lines if needed
	// we need to anticipate if vertical scrollbar will appear.
	// reserve space for continuation indicator
	// reserve space for continuation indicator
	auto& breakLineElem = ctx->theme->getElement(WidgetElementId::MultilineTextInputWordWrap);
	f32 markerWidth = breakLineElem.normalState().width * ctx->scale;
	if (markerWidth <= 0) markerWidth = 4.0f * ctx->scale;
	f32 markerGap = 4.0f * ctx->scale; // gap between text and marker, and right edge
	f32 markerHeight = breakLineElem.normalState().height * ctx->scale;
	if (markerHeight <= 0) markerHeight = bodyElemState->font->getMetrics().height * 0.2f;
	f32 overhangBuffer = 4.0f * ctx->scale;
	f32 indicatorMargin = markerGap + markerWidth + overhangBuffer;
	
	// Use previous frame's scrollbar state to stabilize width and avoid oscillation
	f32 vBarWidth = 0.0f;
	if (state.lastHasScrollbarV && has(flags, MultilineTextInputFlags::WordWrap))
	{
		auto& sbV_check = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();
		vBarWidth = std::ceil(sbV_check.width * ctx->scale);
	}

	f32 effectiveWidth = std::floor(clipRect.width - indicatorMargin - vBarWidth);

	if (state.textChanged || std::abs(state.lastLayoutWidth - effectiveWidth) > 0.5f || state.visualLines.empty())
	{
		printf("multilineTextInput: triggered computeVisualLines. textChanged=%d, width(%.1f -> %.1f), empty=%d\n", 
			(int)state.textChanged, state.lastLayoutWidth, effectiveWidth, (int)state.visualLines.empty());
		state.computeVisualLines(bodyElemState->font, effectiveWidth, rangeHighlights, rangeHighlightCount, keywords, keywordCount);
		state.lastLayoutWidth = effectiveWidth;
	}

	// calculate content height and VBar need for correct wrapping
	f32 totalContentHeight = state.visualLines.size() * lineHeight;
	f32 scrollAreaV = clipRect.height;
	bool hasVerticalScrollbar = totalContentHeight > scrollAreaV;
	state.lastHasScrollbarV = hasVerticalScrollbar;

	// if we need VBar but didn't account for it, or don't need it but did account for it, re-compute
	if (has(flags, MultilineTextInputFlags::WordWrap))
	{
		f32 actualWidth = std::floor(clipRect.width - indicatorMargin - (hasVerticalScrollbar ? vBarWidth : 0.0f));
		if (std::abs(state.lastLayoutWidth - actualWidth) > 0.5f)
		{
			printf("multilineTextInput: re-triggered computeVisualLines (Scrollbar changed). width(%.1f -> %.1f)\n", 
				state.lastLayoutWidth, actualWidth);
			state.computeVisualLines(bodyElemState->font, actualWidth, rangeHighlights, rangeHighlightCount, keywords, keywordCount);
			state.lastLayoutWidth = actualWidth;
			// Height might change after re-wrap
			totalContentHeight = state.visualLines.size() * lineHeight;
		}
	}
	// process Input (Typing, Navigation)
	if (isEditingThis)
	{
		// state.processEvent is handled in beginFrame
	}

	if (isEditingThis && state.textChanged)
	{
		// write back to text buffer
		Utf32String fullText;
		for (size_t i = 0; i < state.lines.size(); i++)
		{
			fullText.insert(fullText.end(), state.lines[i].begin(), state.lines[i].end());
			if (i < state.lines.size() - 1)
				fullText.push_back('\n');
		}

		ctx->settings.services.utf32To8NoAlloc(fullText.data(), (u32)fullText.size(), text, maxLength);
	}

	// draw background
	ctx->renderer.cmdSetColor(bodyElemState->color);

	if (bodyElemState->image)
		ctx->renderer.cmdDrawImageBordered(bodyElemState->image, bodyElemState->border, ctx->widget.rect, ctx->scale);
	else
		ctx->renderer.cmdDrawFilledRectangle(ctx->widget.rect);

	// used cached maxLineWidth
	f32 maxLineWidth = state.maxLineWidth;

	// f32 scrollAreaV = clipRect.height;
	f32 scrollAreaH = clipRect.width;
	hasVerticalScrollbar = false;
	bool hasHorizontalScrollbar = false;

	auto& sbV = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarV).normalState();
	auto& sbH = ctx->theme->getElement(WidgetElementId::ScrollViewScrollBarH).normalState();

	// check if horizontal scrollbar is needed first (simplified logic from ScrollView)
	if (!has(flags, MultilineTextInputFlags::WordWrap) && (maxLineWidth + indicatorMargin) > scrollAreaH)
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
	if (!hasHorizontalScrollbar && (maxLineWidth + indicatorMargin) > scrollAreaH)
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
		auto& lnState = lineNumbersElem.normalState();
		ctx->renderer.cmdSetColor(lnState.color);
		if (lnState.image)
			ctx->renderer.cmdDrawImageBordered(lnState.image, lnState.border, sidebarRect, ctx->scale);
		else
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
	for (i32 i = startLine; i < endLine && i < state.visualLines.size(); i++)
	{
		const auto vl = state.visualLines[i];
		// defensive: skip invalid visual lines
		if (!vl) continue;
		if ((size_t)vl->logicalLineIndex >= state.lines.size()) continue;
		f32 yPos = clipRect.y + i * lineHeight - currentScrollYEarly;

		// use originalClipRect for visibility check because clipRect is indented
		if (yPos + lineHeight < originalClipRect.y || yPos > originalClipRect.bottom())
			continue;

		// current line highlighting
		if (vl->logicalLineIndex == state.currentLine && isEditingThis)
		{
			ctx->renderer.cmdSetColor(currentLineHighlightElemState.color);

			// line number highlight
			if (has(flags, MultilineTextInputFlags::LineNumbers))
			{
				Rect lineNumRect = sidebarRect;
				lineNumRect.y = yPos;
				lineNumRect.height = lineHeight;
				ctx->renderer.cmdDrawFilledRectangle(lineNumRect);
			}

			// text highlight
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

		// draw line number only on first visual line of a logical line
		if (has(flags, MultilineTextInputFlags::LineNumbers) && vl->startColumn == 0)
		{
			// ... (drawing logic below needs to be inside loop or separate?)
			// the original code had separate loops for highlights and numbers?
			// let's check original code.
			// original code: LOOP 1 draws highlights. LOOP 2 draws numbers (lines 658+).
			// this loop (lines 322-364) seems to draw highlights AND potentially line number background?
			// yes, line 338 draws "line number highlight".
			// line numbers themselves are drawn later (line 658).
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
		if ((maxLineWidth + indicatorMargin) > availableWidth)
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
		if (!hasHorizontalScrollbar && (maxLineWidth + indicatorMargin) > availableWidth)
		{
			hasHorizontalScrollbar = true;
			scrollAreaV -= sbH.height * ctx->scale;
			// re-evaluate Vertical with reduced height (optional, but consistent)
			if (!hasVerticalScrollbar && totalContentHeight > scrollAreaV)
				hasVerticalScrollbar = true;
		}

	// vertical scrollbar rect (right side of clipRect)
		Rect vRect;
		if (hasVerticalScrollbar)
		{
			// vertical scrollbar rect (right side of clipRect)
			vRect = clipRect;
			vRect.x = vRect.right() - sbV.width * ctx->scale;
			vRect.width = sbV.width * ctx->scale;
			// be careful: ScrollView might deduce height if H-scroll is present
			if (hasHorizontalScrollbar)
				vRect.height -= sbH.height * ctx->scale;

			if (vRect.contains(ctx->mousePosition))
				overScrollbar = true;
		}

		Rect hRect;
		if (hasHorizontalScrollbar)
		{
			// horizontal scrollbar rect (bottom of clipRect)
			hRect = clipRect;
			hRect.y = hRect.bottom() - sbH.height * ctx->scale;
			hRect.height = sbH.height * ctx->scale;
			// be careful: if V-scroll is present, full width might be reduced?
			// in ScrollView:
			// f32 scrollBarWidthFull = rectNoBorders.width;
			// if (scrollContentSizeV > ...) scrollBarWidthFull -= ...;
			// rect rectScrollBarH = { ..., scrollBarWidthFull, ... }
			// so yes, H scrollbar doesn't extend under V scrollbar usually.
			if (hasVerticalScrollbar)
				hRect.width -= sbV.width * ctx->scale;

			if (hRect.contains(ctx->mousePosition))
				overScrollbar = true;
		}

		if (state.id == ctx->id)
		{
			state.scrollbarRectV = hasVerticalScrollbar ? vRect : Rect();
			state.scrollbarRectH = hasHorizontalScrollbar ? hRect : Rect();
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

	// fix 2: constrain ScrollView width to the clipRect width (since we indented position)
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
			initialScroll = it->second.scrollOffset;
	}

	// sync state scroll offset immediately so subsequent calculations (like getCaretScreenPosition) are correct for this frame
	state.scrollOffsetX = initialScroll.x;
	state.scrollOffsetY = initialScroll.y;

	// prevent scroll "jump" when deleting lines from the end:
	// ensure content height is at least (currentScroll + viewHeight) so ScrollView doesn't clamp it up.
	// but ONLY do this if the real content is large enough to warrant scrolling (i.e. > viewHeight).
	// if the entire text fits in the view, let it snap to top naturally.
	// use pixel height for consistent drift logic comparisons
	f32 viewPixelHeight = clipRect.height;

	if (totalContentHeight > viewPixelHeight)
	{
		if (totalContentHeight < initialScroll.y + viewPixelHeight)
		{
			// smart Drift Buffer Refined:
			// "freeze" the view ONLY if we have more than 1 line of NON-EMPTY context visible starting from the scroll position.
			// this addresses the issue where trailing empty lines were counting as context, preventing scroll up.
			// we iterate through lines starting from the current scroll position to find 2 non-empty lines.

			int visibleNonEmptyLines = 0;
			size_t caretVisualLineIndex = state.caretVisualLineIndex;

			if (lineHeight > 0)
			{
				// fix: Use VISUAL lines for iteration, not logical lines directly,
				// because scroll position is in visual units.
				size_t firstVisualLineIndex = (size_t)(initialScroll.y / lineHeight);

				// limit scan to 20 lines to avoid O(N) hit on 100k+ line files
				size_t scanLimit = std::min(state.visualLines.size(), firstVisualLineIndex + 20);

				for (size_t i = firstVisualLineIndex; i < scanLimit; ++i)
				{
					const auto vl = state.visualLines[i];
					// defensive: skip invalid visual lines
					if (!vl) continue;
					if ((size_t)vl->logicalLineIndex >= state.lines.size()) continue;
					const auto& lineText = state.lines[vl->logicalLineIndex];

					// check if this visual segment has meaningful content
					// we need to check the specific substring for this visual line
					bool hasContent = false;

					// optimization: Just check if the segment length > 0 and if it contains non-whitespace
					if (vl->length > 0)
					{
						for (size_t c = 0; c < vl->length; c++)
						{
							u32 ch = lineText[vl->startColumn + c];
							if (ch > 32 && ch != 160)
							{
								hasContent = true;
								break;
							}
						}
					}
					// special case: If it's an empty logical line (length 0), it might be wrapping?
					// no, visual lines for successful wrap shouldn't be empty unless logical line is empty.
					// if logical line is empty, it has 0 length.

					// force content if caret is here
					if (isEditingThis && i == caretVisualLineIndex)
					{
						hasContent = true;
					}

					if (hasContent)
					{
						visibleNonEmptyLines++;
						if (visibleNonEmptyLines >= 2)
							break;
					}
				}
			}

			// check if caret is at the top of the visible area (Top 2.5 lines to be safe and generous)
			bool caretAtTop = false;
			Point caretPos = state.getCaretScreenPosition();
			if (caretPos.y < clipRect.y + lineHeight * 2.5f)
				caretAtTop = true;

			// freeze ONLY if we have context AND the caret is not at the top edge (user sees context above).
			if (visibleNonEmptyLines >= 2 && !caretAtTop)
			{
				totalContentHeight = initialScroll.y + viewPixelHeight; // freeze
			}
			else
			{
				// "drift Up": Reduce padding to allow scroll to decrease by 1 line/frame
				// until we find content + buffer (2 non-empty lines) OR until caret is not at top.
				f32 paddedHeight = initialScroll.y + viewPixelHeight - lineHeight;
				if (paddedHeight > totalContentHeight)
					totalContentHeight = paddedHeight;
			}
		}
	}

	// removed redundant else { initialScroll = {0,0} } block
	// because beginScrollView handles clamping if content is smaller than view.
	// this prevents accidental jumps if height comparison is fuzzy.

	beginScrollView(scrollIdName.c_str(), scrollViewHeight, initialScroll, { maxLineWidth + indicatorMargin, totalContentHeight }, ScrollViewFlags::NoBorder | ScrollViewFlags::NoPadding);


	ctx->layout.width = savedLayoutWidth; // restore layout width immediately (beginScrollView captured it)

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
		auto sel_start = std::chrono::high_resolution_clock::now();
		i32 startLine = state.selectionStartLine;
		i32 startCol = state.selectionStartColumn;
		i32 endLine = state.selectionEndLine;
		i32 endCol = state.selectionEndColumn;

		if (startLine > endLine || (startLine == endLine && startCol > endCol))
		{
			std::swap(startLine, endLine);
			std::swap(startCol, endCol);
		}

		for (i32 i = firstLine; i < lastLine && i < (i32)state.visualLines.size(); i++)
		{
			const auto vl = state.visualLines[i];
			// defensive: skip invalid visual lines
			if (!vl) continue;
			if ((size_t)vl->logicalLineIndex >= state.lines.size()) continue;
			f32 yPos = clipRect.y + i * lineHeight - currentScrollY;

			if (yPos + lineHeight < clipRect.y || yPos > clipRect.bottom())
				continue;

				// check if this visual line is within the selection range
			if (vl->logicalLineIndex < startLine || vl->logicalLineIndex > endLine)
				continue;

			i32 selStartOnLine = 0;
			i32 selEndOnLine = vl->length;

			if (vl->logicalLineIndex == startLine)
			{
				if (startCol >= vl->startColumn + vl->length) continue; // selection starts after this segment
				selStartOnLine = std::max(0, startCol - vl->startColumn);
			}

			if (vl->logicalLineIndex == endLine)
			{
				if (endCol <= vl->startColumn) continue; // selection ends before this segment
				selEndOnLine = std::min(vl->length, endCol - vl->startColumn);
			}

			// handle case where we select the newline character (effectively selecting past the end)
			// in visual lines, likely only the last segment of a logical line should visualize newline selection?
			// if vl is the last segment of a logical line:
			// make sure startColumn index is valid for the referenced logical line
			auto& logicalLine = state.lines[vl->logicalLineIndex];
			bool isLastSegment = (vl->startColumn + vl->length == (i32)logicalLine.size());
			bool selectingNewline = false;

			if (vl->logicalLineIndex < endLine && isLastSegment)
			{
				// we are selecting past this line, so we are selecting the newline
				selectingNewline = true;
			}
			else if (vl->logicalLineIndex == endLine && (size_t)endCol == logicalLine.size() && isLastSegment)
			{
				// explicitly selecting to end of line
				selectingNewline = false; // usually standard editors don't select newline if just at end, unless endLine > currentLine
				// actually if startLine != endLine, then this line is fully selected including newline.
				if (startLine != endLine) selectingNewline = true;
			}

			if (selStartOnLine >= selEndOnLine && !selectingNewline)
				continue;

			Utf32String textToStart(logicalLine.begin() + vl->startColumn, logicalLine.begin() + vl->startColumn + selStartOnLine);
			Utf32String selectedText(logicalLine.begin() + vl->startColumn + selStartOnLine, logicalLine.begin() + vl->startColumn + selEndOnLine);

			FontTextSize toStartSize = font->computeTextSize(textToStart.data(), (u32)textToStart.size());
			FontTextSize selectedSize = font->computeTextSize(selectedText.data(), (u32)selectedText.size());

			Rect selRect;
			selRect.x = clipRect.x + toStartSize.width - currentScrollX;
			selRect.y = yPos;
			selRect.width = selectedSize.width;
			selRect.height = lineHeight;

			if (selectingNewline)
			{
				static f32 spaceWidth = 0.0f;
				if (spaceWidth == 0.0f)
					spaceWidth = font->computeTextSize(" ", 1).width;
				selRect.width += spaceWidth;
			}

			// if selection width is 0 (point selection? should be handled by loop check)
			// but if just newline is selected:
			if (selRect.width <= 0.001f && selectingNewline)
			{
				static f32 spaceWidth = 0.0f;
				if (spaceWidth == 0.0f)
					spaceWidth = font->computeTextSize(" ", 1).width;
				selRect.width = spaceWidth;
			}

			if (selRect.width > 0)
			{
				ctx->renderer.cmdSetColor(bodyTextSelectionElemState.color);
				ctx->renderer.cmdDrawFilledRectangle(selRect);
			}
		}
		auto sel_end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> sel_elapsed = sel_end - sel_start;
		if (sel_elapsed.count() > 0.05) printf("Selection rendering took %.3f ms\n", sel_elapsed.count());
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

	// draw line numbers
	if (has(flags, MultilineTextInputFlags::LineNumbers))
	{
		ctx->renderer.popClipRect(); // pop inner clip to draw in sidebar

		// ensure we clip line numbers to the widget's vertical bounds to avoid drawing outside
		Rect sidebarClip = originalClipRect;
		sidebarClip.x = originalClipRect.x - sidebarWidth; // include sidebar in clip X
		sidebarClip.width += sidebarWidth;
		ctx->renderer.pushClipRect(sidebarClip);

		auto& lnState = lineNumbersElem.normalState();
		ctx->renderer.cmdSetColor(lnState.textColor);
		ctx->renderer.cmdSetFont(lnState.font);

		for (i32 i = firstLine; i < lastLine && i < (i32)state.visualLines.size(); i++)
		{
			const auto vl = state.visualLines[i];
			// defensive: skip invalid visual lines
			if (!vl) continue;
			if ((size_t)vl->logicalLineIndex >= state.lines.size()) continue;
			f32 yPos = clipRect.y + i * lineHeight - currentScrollY;

			if (yPos + lineHeight < clipRect.y || yPos > clipRect.bottom())
				continue;

			// only draw line number for the first segment of a logical line
			if (vl->startColumn == 0)
			{
				char numStr[32];
				sprintf(numStr, "%d", vl->logicalLineIndex + 1);

				Rect lnRect;
				lnRect.x = clipRect.x - sidebarWidth + 5.0f;
				lnRect.y = yPos;
				lnRect.width = sidebarWidth - 10.0f;
				lnRect.height = lineHeight;

				ctx->renderer.cmdDrawTextInBox(
					numStr,
					lnRect,
					HAlignType::Right,
					VAlignType::Bottom, false, true);
			}
		}

		ctx->renderer.popClipRect(); // pop sidebar clip
		ctx->renderer.cmdSetFont(bodyElemState->font); // restore font
		ctx->renderer.pushClipRect(clipRect); // restore inner clip
	}


	// draw text
	auto text_start = std::chrono::high_resolution_clock::now();
	for (i32 i = firstLine; i < lastLine && i < (i32)state.visualLines.size(); i++)
	{
		const auto vl = state.visualLines[i];
		// defensive: skip invalid visual lines
		if (!vl) continue;
		if ((size_t)vl->logicalLineIndex >= state.lines.size()) continue;
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

		// extract segment text
		// utf32String doesn't have substr, construct from iterator range
		// ensure startColumn is valid for the referenced logical line
		auto& logicLine = state.lines[vl->logicalLineIndex];
		if ((size_t)vl->startColumn >= logicLine.size())
			continue; // nothing to draw for this visual segment (defensive)
		size_t safelyEnd = std::min(logicLine.size(), (size_t)(vl->startColumn + vl->length));
		
		// Use persistent buffer to avoid per-line allocations
		state.utf8LineBuffer.resize((safelyEnd - vl->startColumn) * 4 + 1);
		char* lineText = state.utf8LineBuffer.data();
		ctx->settings.services.utf32To8NoAlloc(logicLine.data() + vl->startColumn, (u32)(safelyEnd - vl->startColumn), lineText, (u32)state.utf8LineBuffer.size());

		if (lineText && lineText[0])
		{
			f32 currentX = textRect.x;
			i32 vsegOffset = 0;

			for (const auto& vseg : vl->segments)
			{
				if (vseg.length <= 0) continue;

				char* vsegTextBuf = state.utf8LineBuffer.data();
				// Use the persistent buffer for this specific segment to ensure null-termination
				ctx->settings.services.utf32To8NoAlloc(logicLine.data() + vl->startColumn + vsegOffset, (u32)vseg.length, vsegTextBuf, (u32)state.utf8LineBuffer.size());

				ctx->renderer.cmdSetColor(vseg.color);
				Rect segRect = textRect;
				segRect.x = currentX;
				ctx->renderer.cmdDrawTextInBox(
					vsegTextBuf,
					segRect,
					HAlignType::Left,
					VAlignType::Bottom, false, true);

				currentX += font->computeTextSize(vsegTextBuf).width;
				vsegOffset += vseg.length;
			}
		}

		// draw continuation symbol if this segment is followed by more content on the same logical line
		if (vl->startColumn + vl->length < (i32)logicLine.size())
		{
			// draw simple filled rect at end of line
			// x position is fixed to the right side of the view area

			Rect contRect;
			// draw it exactly at the right boundary of the measured visual layout (which grows/shrinks cleanly using the scrollbar checks at the start of frame)
			contRect.x = clipRect.x + state.lastLayoutWidth + markerGap;
			contRect.y = yPos + (lineHeight - markerHeight) / 2.0f;
			contRect.width = markerWidth;
			contRect.height = markerHeight;

			ctx->renderer.cmdSetColor(breakLineElem.normalState().color);

			if (breakLineElem.normalState().image)
			{
				ctx->renderer.cmdDrawImage(breakLineElem.normalState().image, contRect);
			}
			else
			{
				ctx->renderer.cmdDrawFilledRectangle(contRect);
			}
		}
			// end of logical line.
		}
	auto text_end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> text_elapsed = text_end - text_start;
	if (text_elapsed.count() > 0.05) printf("Text rendering took %.3f ms\n", text_elapsed.count());

	// advance layout position so ScrollView knows the content height
	ctx->position.y += totalContentHeight;

	endScrollView();
	ctx->position = wrapperEndPos; // restore layout position
	// fix 3: Removed unbalanced popClipRect() here (beginScrollView handles its own push/pop)

	setFocusable();

	if (ctx->settings.textCaretBlinkSpeed > 0 && ctx->widget.focused)
	{
		state.caretBlinkTimer += ctx->settings.deltaTime * ctx->settings.textCaretBlinkSpeed;

		if (state.caretBlinkTimer > 2.0f)
			state.caretBlinkTimer = 0;
	}

	popLayout();

	bool changed = state.textChanged;
	state.textChanged = false;
	state.firstDirtyLine = -1;
	state.forceLayoutUpdate = false;
	return changed;
}

}
