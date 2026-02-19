#include "multiline_text_input_state.h"
#include "util.h"
#include "font.h"
#include "context.h"
#include "theme.h"

namespace hui
{
MultilineTextInputState::MultilineTextInputState()
{
	lines.push_back(Utf32String()); // start with one empty line
}

void MultilineTextInputState::selectAll()
{
	selectionActive = true;
	selectionStartLine = 0;
	selectionStartColumn = 0;
	selectionEndLine = lines.size() - 1;
	selectionEndColumn = lines.back().size();
	currentLine = selectionEndLine;
	caretColumn = selectionEndColumn;
	selectAllOnFocus = false;
}

void MultilineTextInputState::deselect()
{
	selectionActive = false;
	selectionStartLine = selectionStartColumn = 0;
	selectionEndLine = selectionEndColumn = 0;
}

void MultilineTextInputState::deleteSelection()
{
	if (!selectionActive)
		return;

	// normalize selection
	i32 startLine = selectionStartLine;
	i32 startCol = selectionStartColumn;
	i32 endLine = selectionEndLine;
	i32 endCol = selectionEndColumn;

	if (startLine > endLine || (startLine == endLine && startCol > endCol))
	{
		std::swap(startLine, endLine);
		std::swap(startCol, endCol);
	}

	if (startLine == endLine)
	{
		// single line deletion
		lines[startLine].erase(lines[startLine].begin() + startCol, lines[startLine].begin() + endCol);
	}
	else
	{
		// multi-line deletion
		// keep text before selection on start line and text after selection on end line
		Utf32String remainingText = Utf32String(lines[startLine].begin(), lines[startLine].begin() + startCol);
		Utf32String afterSelection(lines[endLine].begin() + endCol, lines[endLine].end());
		remainingText.insert(remainingText.end(), afterSelection.begin(), afterSelection.end());

		// remove all lines in between
		lines.erase(lines.begin() + startLine, lines.begin() + endLine + 1);
		lines.insert(lines.begin() + startLine, remainingText);
	}

	currentLine = startLine;
	caretColumn = startCol;
	deselect();
	textChanged = true;
}

Utf32String MultilineTextInputState::getSelection()
{
	if (!selectionActive)
		return Utf32String();

	i32 startLine = selectionStartLine;
	i32 startCol = selectionStartColumn;
	i32 endLine = selectionEndLine;
	i32 endCol = selectionEndColumn;

	if (startLine > endLine || (startLine == endLine && startCol > endCol))
	{
		std::swap(startLine, endLine);
		std::swap(startCol, endCol);
	}

	if (startLine == endLine)
	{
		return Utf32String(lines[startLine].begin() + startCol, lines[startLine].begin() + endCol);
	}

	// multi-line selection
	Utf32String result;
	for (i32 i = startLine; i <= endLine; i++)
	{
		if (i == startLine)
		{
			Utf32String part(lines[i].begin() + startCol, lines[i].end());
			result.insert(result.end(), part.begin(), part.end());
		}
		else if (i == endLine)
		{
			Utf32String part(lines[i].begin(), lines[i].begin() + endCol);
			result.insert(result.end(), part.begin(), part.end());
		}
		else
		{
			result.insert(result.end(), lines[i].begin(), lines[i].end());
		}

		if (i < endLine)
			result.push_back('\n');
	}

	return result;
}

void MultilineTextInputState::clearText()
{
	lines.clear();
	lines.push_back(Utf32String());
	currentLine = 0;
	caretColumn = 0;
	deselect();
	scrollOffsetX = scrollOffsetY = 0;
	textChanged = true;
}

void MultilineTextInputState::insertTextAtCaret(const Utf32String& newText)
{
	if (selectionActive)
		deleteSelection();

	if (newText.empty())
		return;

	// calculate current total length
	size_t totalLength = 0;
	for (const auto& line : lines)
		totalLength += line.size();
	if (!lines.empty())
		totalLength += lines.size() - 1; // count newlines

	if (totalLength >= maxTextLength)
		return;

	// handle newlines in pasted text
	for (u32 ch : newText)
	{
		// check limit
		if (totalLength >= maxTextLength)
			break;

		if (ch == '\r')
		{
			// ignore carriage return, wait for newline
			continue;
		}
		else if (ch == '\n')
		{
			// split line at caret
			Utf32String remaining(lines[currentLine].begin() + caretColumn, lines[currentLine].end());
			lines[currentLine].erase(lines[currentLine].begin() + caretColumn, lines[currentLine].end());
			currentLine++;
			lines.insert(lines.begin() + currentLine, remaining);
			caretColumn = 0;
			totalLength++;
		}
		else if (ch == '\t')
		{
			if (has(flags, MultilineTextInputFlags::SpacesOnTab))
			{
				u32 tabSize = ctx->settings.tabSize;

				// expand tab to N spaces
				for (u32 k = 0; k < tabSize; k++)
				{
					if (totalLength >= maxTextLength)
						break;

					lines[currentLine].insert(lines[currentLine].begin() + caretColumn, ' ');
					caretColumn++;
					totalLength++;
				}
			}
			else
			{
				lines[currentLine].insert(lines[currentLine].begin() + caretColumn, '\t');
				caretColumn++;
				totalLength++;
			}
		}
		else
		{
			lines[currentLine].insert(lines[currentLine].begin() + caretColumn, ch);
			caretColumn++;
			totalLength++;
		}
	}

	textChanged = true;
}

Point MultilineTextInputState::getCaretScreenPosition()
{
	auto& elemState = themeElement->normalState();
	Font* font = elemState.font;

	if (!font || lines.empty())
		return Point(clipRect.x, clipRect.y);

	f32 lineHeight = font->getMetrics().height;

	i32 foundVisualLine = -1;

	for (size_t i = 0; i < visualLines.size(); ++i)
	{
		const auto& vl = visualLines[i];
		if (vl.logicalLineIndex == currentLine)
		{
			bool isLastSegment = true;
			if (i + 1 < visualLines.size() && visualLines[i+1].logicalLineIndex == currentLine)
				isLastSegment = false;

			if (caretColumn >= vl.startColumn && caretColumn < vl.startColumn + vl.length)
			{
				foundVisualLine = (i32)i;
				break;
			}

			// Ambiguous case: caret is at the end of this visual line (which equals start of next if wrapped).
			// If caretPreferLineEnd is true, we snap to this line.
			// If isLastSegment (end of logical line), we always snap to this line.
			if (caretColumn == vl.startColumn + vl.length)
			{
				if (isLastSegment || caretPreferLineEnd)
				{
					foundVisualLine = (i32)i;
					break;
				}
			}
		}
	}

	if (foundVisualLine == -1 && !visualLines.empty())
	{
		if (currentLine >= lines.size()) foundVisualLine = (i32)visualLines.size() - 1;
		else
		{
			for (i32 i = (i32)visualLines.size() - 1; i >= 0; i--)
			{
				if (visualLines[i].logicalLineIndex == currentLine)
				{
					foundVisualLine = i;
					break;
				}
			}
		}
	}

	if (foundVisualLine == -1)
		return Point(clipRect.x - scrollOffsetX, clipRect.y - scrollOffsetY);

	const auto& vl = visualLines[foundVisualLine];
	Utf32String textStr = lines[currentLine];

	i32 relCaret = caretColumn - vl.startColumn;
	if (relCaret < 0) relCaret = 0;
	if (relCaret > vl.length) relCaret = vl.length;

	Utf32String segText(textStr.begin() + vl.startColumn, textStr.begin() + vl.startColumn + relCaret);
	f32 xOffset = font->computeTextSize(segText.data(), (u32)segText.size()).width;

	return Point(
		clipRect.x + xOffset - scrollOffsetX,
		clipRect.y + foundVisualLine * lineHeight - scrollOffsetY
	);
}

void MultilineTextInputState::computeScrollAmount()
{
	// ensure we have the latest scroll offset before calculations
	if (scrollId)
	{
		auto& scrollState = ctx->scrollViewState[scrollId];
		scrollOffsetX = scrollState.scrollOffset.x;
		scrollOffsetY = scrollState.scrollOffset.y;
	}

	Point caretPos = getCaretScreenPosition();
	auto& elemState = themeElement->normalState();
	Font* font = elemState.font;
	f32 lineHeight = font ? font->getMetrics().height : 20.0f;

	// horizontal scrolling
	if (caretPos.x < clipRect.x)
		scrollOffsetX -= (clipRect.x - caretPos.x) + 10;
	else if (caretPos.x > clipRect.right())
		scrollOffsetX += (caretPos.x - clipRect.right()) + 10;

	if (scrollOffsetX < 0)
		scrollOffsetX = 0;

	// vertical scrolling
	if (caretPos.y < clipRect.y)
		scrollOffsetY -= (clipRect.y - caretPos.y); // Snap exactly to top
	else if (caretPos.y + lineHeight > clipRect.bottom())
		scrollOffsetY += (caretPos.y + lineHeight - clipRect.bottom()); // Snap exactly to bottom

	if (scrollOffsetY < 0)
		scrollOffsetY = 0;

	// update context scroll state and force repaint if changed
	if (scrollId)
	{
		auto& scrollState = ctx->scrollViewState[scrollId];
		if (scrollState.scrollOffset.y != scrollOffsetY || scrollState.scrollOffset.x != scrollOffsetX)
		{
			scrollState.scrollOffset.x = scrollOffsetX;
			scrollState.scrollOffset.y = scrollOffsetY;
			// also update the axis-specific state which ScrollView logic relies on
			scrollState.horizontal.scrollOffset = scrollOffsetX;
			scrollState.vertical.scrollOffset = scrollOffsetY;
			forceRepaint();
		}
	}
}

void MultilineTextInputState::ensureCaretVisible()
{
	if (textChanged && lastLayoutWidth > 0 && themeElement)
	{
		Font* font = themeElement->normalState().font;
		if (font)
		{
			computeVisualLines(font, lastLayoutWidth);
		}
	}
	computeScrollAmount();
}

void MultilineTextInputState::formatValue()
{
	// no formatting for multiline (could add later if needed)
}

i32 MultilineTextInputState::getCharIndexAtPoint(const Point& pt)
{
	// sync scroll offset from context if available
	if (scrollId)
	{
		auto& scrollState = ctx->scrollViewState[scrollId];
		scrollOffsetX = scrollState.scrollOffset.x;
		scrollOffsetY = scrollState.scrollOffset.y;
	}

	Font* font = themeElement ? themeElement->normalState().font : nullptr;
	if (!font || visualLines.empty())
		return 0;

	f32 lineHeight = font->getMetrics().height;

	// ensure adjustedY is at least 0 to prevent negative line indexing
	f32 adjustedY = pt.y + scrollOffsetY - clipRect.y;
	if (adjustedY < 0) adjustedY = 0;

	i32 visualLineIdx = (i32)(adjustedY / lineHeight);

	if (visualLineIdx < 0) visualLineIdx = 0;
	if (visualLineIdx >= visualLines.size()) visualLineIdx = (i32)visualLines.size() - 1;

	const VisualLine& vl = visualLines[visualLineIdx];
	currentLine = vl.logicalLineIndex;

	// find column within this visual segment
	f32 adjustedX = pt.x + scrollOffsetX;

	// The text on this visual line is a substring of logical line
	const auto& lineText = lines[currentLine];

	// Optimization: Only measure the substring for this visual line
	Utf32String segmentText(lineText.begin() + vl.startColumn, lineText.begin() + vl.startColumn + vl.length);

	// Using binary search or linear scan for character position?
	// Linear scan is acceptable for now.
	for (size_t i = 0; i <= segmentText.size(); i++)
	{
		Utf32String substr(segmentText.begin(), segmentText.begin() + i);
		FontTextSize size = font->computeTextSize(substr.data(), (u32)substr.size());

		if (clipRect.x + size.width >= adjustedX)
		{
			// found split point
			// closer to this char or the previous one?
			// standard behavior is hit testing.
			i32 relCol = (i > 0) ? (i32)i - 1 : 0;
			// check if click is past half width of char?
			// Ignoring half-width check for simplicity to match previous style
			caretColumn = vl.startColumn + relCol;
			return caretColumn;
		}
	}

	// if past end of visual line's content, place at end of visual line
	caretColumn = vl.startColumn + vl.length;
	return caretColumn;
}

void MultilineTextInputState::computeVisualLines(Font* font, f32 availableWidth)
{
	visualLines.clear();

	if (!font || lines.empty())
		return;

	if (!has(flags, MultilineTextInputFlags::WordWrap))
	{
		for (size_t i = 0; i < lines.size(); ++i)
		{
			VisualLine vl;
			vl.logicalLineIndex = (i32)i;
			vl.startColumn = 0;
			vl.length = (i32)lines[i].size();
			vl.width = font->computeTextSize(lines[i].data(), (u32)lines[i].size()).width;
			visualLines.push_back(vl);
		}
		return;
	}

	for (size_t i = 0; i < lines.size(); ++i)
	{
		const Utf32String& line = lines[i];

		if (line.empty())
		{
			VisualLine vl;
			vl.logicalLineIndex = (i32)i;
			vl.startColumn = 0;
			vl.length = 0;
			vl.width = 0;
			visualLines.push_back(vl);
			continue;
		}

		i32 currentStart = 0;

		while (currentStart < line.size())
		{
			i32 remaining = (i32)line.size() - currentStart;
			i32 bestLength = 0;

			// Binary search for length that fits
			i32 low = 1;
			i32 high = remaining;
			i32 fitLength = 0;
			FontTextSize textSize;

			// Optimization: Start closer to expected length if possible?
			// For now standard binary search on substring length.

			// To avoid O(N log N) text measurement, we can just measure char by char? No, shaping.
			// Binary search is reasonable.
			while (low <= high)
			{
				i32 mid = low + (high - low) / 2;
				// TODO: Avoid alloc here
				Utf32String sub(line.begin() + currentStart, line.begin() + currentStart + mid);
				textSize = font->computeTextSize(sub.data(), (u32)sub.size());

				if (textSize.width <= availableWidth)
				{
					fitLength = mid;
					low = mid + 1;
				}
				else
				{
					high = mid - 1;
				}
			}

			if (fitLength == 0 && remaining > 0)
			{
				// If strictly nothing fits (e.g. one very wide char > width), force 1 char
				fitLength = 1;
			}

			i32 lengthOnLine = fitLength;
			bool forceWrap = false;

			// If we are wrapping (not at end of line), try to break at space
			if (currentStart + fitLength < line.size())
			{
				bool foundBreak = false;
				i32 breakIdx = fitLength;

				// search backwards for space
				for (i32 k = fitLength; k > 0; k--)
				{
					// Check char at index (currentStart + k - 1)
					u32 ch = line[currentStart + k - 1];
					if (ch == ' ' || ch == '\t' || ch == '-')
					{
						breakIdx = k;
						foundBreak = true;
						break;
					}
				}

				if (foundBreak)
				{
					lengthOnLine = breakIdx;
				}
			}

			VisualLine vl;
			vl.logicalLineIndex = (i32)i;
			vl.startColumn = currentStart;
			vl.length = lengthOnLine;
			// Measure actual width of this segment for alignment/layout (optional but good)
			// avoiding alloc again if possible, but state update is rare compared to draw
			Utf32String seg(line.begin() + currentStart, line.begin() + currentStart + lengthOnLine);
			vl.width = font->computeTextSize(seg.data(), (u32)seg.size()).width;

			visualLines.push_back(vl);

			currentStart += lengthOnLine;
		}
	}
}

bool MultilineTextInputState::processEvent(const InputEvent& ev)
{
	textChanged = false;

	if (!id)
		return false;

	const Point& mousePos = ctx->mousePosition;

	if (ev.type == InputEvent::Type::MouseDown)
	{
		if (!rect.contains(ev.mouse.point))
		{
			id = 0;
			return false;
		}

		// if click is within widget bounds but outside clip rect (e.g. on scrollbar),
		// ignore it here so ScrollView can handle it, but keep focus (don't clear id).
		if (!clipRect.contains(ev.mouse.point)
			|| scrollbarRectV.contains(ev.mouse.point)
			|| scrollbarRectH.contains(ev.mouse.point))
		{
			return false;
		}

		mouseDown = true;
		getCharIndexAtPoint(ev.mouse.point);
		mouseDownSelectionStartLine = currentLine;
		mouseDownSelectionStartColumn = caretColumn;
		mouseMoved = false;
		selectionStartLine = selectionEndLine = currentLine;
		selectionStartColumn = selectionEndColumn = caretColumn;
		selectingWithMouse = false;
		ensureCaretVisible();
		setWindowCapture();
	}
	else if (ev.type == InputEvent::Type::MouseUp)
	{
		if (mouseDown || selectingWithMouse)
			ensureCaretVisible();

		releaseWindowCapture();

		if (!mouseMoved && firstMouseDown && !selectingWithMouse)
		{
			if (has(flags, MultilineTextInputFlags::AutoSelectAll) && selectAllOnFocus)
				selectAll();
			firstMouseDown = false;
		}

		mouseDown = false;
		selectingWithMouse = false;
	}
	else if (ev.window == ctx->settings.services.getFocusedWindow())
	{
		if (mouseDown || selectingWithMouse)
		{
			getCharIndexAtPoint(ev.mouse.point);

			if (mouseDown)
			{
				if (currentLine != mouseDownSelectionStartLine || caretColumn != mouseDownSelectionStartColumn)
					mouseMoved = true;
				selectingWithMouse = true;
				mouseDown = false;
			}

			if (selectingWithMouse)
			{
				selectionEndLine = currentLine;
				selectionEndColumn = caretColumn;
				selectionActive = true;
				ensureCaretVisible();
			}
		}
	}

	if (ev.type == InputEvent::Type::Text)
	{
		caretBlinkTimer = 0;
		Utf32String txt;
		ctx->settings.services.utf8To32(ev.text.text, txt);

		// strictly filter out newlines - if any newline char is present,
		// assume it is handled by Key event and do nothing here.
		for (u32 ch : txt)
		{
			if (ch == '\n' || ch == '\r')
				return true; // Ignore this event completely
		}

		insertTextAtCaret(txt);
		textChanged = true;
	}
	else if (ev.type == InputEvent::Type::Key)
	{
		processKeyEvent(ev);
	}

	return true;
}

void MultilineTextInputState::processKeyEvent(const InputEvent& ev)
{
	if (ev.type != InputEvent::Type::Key || !ev.key.down)
		return;

	caretBlinkTimer = 0;

	Font* font = themeElement ? themeElement->normalState().font : nullptr;
	bool hasSelection = selectionActive;

	if (ev.key.code == KeyCode::ArrowLeft)
	{
		caretPreferLineEnd = false;
		i32 prevLine = currentLine;
		i32 prevColumn = caretColumn;

		if (has(ev.key.modifiers, KeyModifiers::Control))
		{
			if (caretColumn == 0)
			{
				if (currentLine > 0)
				{
					currentLine--;
					caretColumn = lines[currentLine].size();
				}
			}
			else
			{
				// 1. Skip preceding whitespace
				while (caretColumn > 0 && isspace(lines[currentLine][caretColumn - 1]))
					caretColumn--;

				// 2. Skip preceding non-whitespace
				while (caretColumn > 0 && !isspace(lines[currentLine][caretColumn - 1]))
					caretColumn--;
			}
		}
		else
		{
			if (caretColumn > 0)
				caretColumn--;
			else if (currentLine > 0)
			{
				currentLine--;
				caretColumn = lines[currentLine].size();
			}
		}

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = prevLine;
				selectionStartColumn = prevColumn;
			}
			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
			deselect();
	}
	else if (ev.key.code == KeyCode::ArrowRight)
	{
		caretPreferLineEnd = false;
		i32 prevLine = currentLine;
		i32 prevColumn = caretColumn;

		if (has(ev.key.modifiers, KeyModifiers::Control))
		{
			if (caretColumn >= lines[currentLine].size())
			{
				if (currentLine < lines.size() - 1)
				{
					currentLine++;
					caretColumn = 0;
				}
			}
			else
			{
				// 1. Skip succeeding non-whitespace
				while (caretColumn < lines[currentLine].size() && !isspace(lines[currentLine][caretColumn]))
					caretColumn++;

				// 2. Skip succeeding whitespace
				while (caretColumn < lines[currentLine].size() && isspace(lines[currentLine][caretColumn]))
					caretColumn++;
			}
		}
		else
		{
			if (caretColumn < lines[currentLine].size())
				caretColumn++;
			else if (currentLine < lines.size() - 1)
			{
				currentLine++;
				caretColumn = 0;
			}
		}

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = prevLine;
				selectionStartColumn = prevColumn;
			}
			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
			deselect();
	}
	else if (ev.key.code == KeyCode::ArrowUp)
	{
		i32 prevLine = currentLine;
		i32 prevColumn = caretColumn;

		// Move visually up
		if (!visualLines.empty())
		{
			i32 vIdx = -1;
			// Find current visual line
			for (size_t i = 0; i < visualLines.size(); ++i)
			{
				const auto& vl = visualLines[i];
				if (vl.logicalLineIndex == currentLine)
				{
					if (caretColumn >= vl.startColumn && caretColumn <= vl.startColumn + vl.length)
					{
						vIdx = (i32)i;
						if (caretColumn < vl.startColumn + vl.length)
						{
							vIdx = (i32)i;
							break;
						}
						// Ambiguous case: caret is exactly at split point (end of this line, start of next)
						// If isLastSegment, it's definitely this line.
						// If caretPreferLineEnd is true, we want this line.
						// Otherwise we prioritize the NEXT line (which will be found in next iteration).
						bool isLastSegment = (i + 1 >= visualLines.size() || visualLines[i+1].logicalLineIndex != currentLine);
						if (isLastSegment || caretPreferLineEnd)
						{
							vIdx = (i32)i;
							break;
						}
					}
				}
			}
			// Reset affinity after moving off the line
			caretPreferLineEnd = false;
			// Use last match if ambiguous (e.g. end of line) and we are at end of segment?
			// Actually getCaretScreenPosition logic was specific.
			// Simple logic: if caretColumn is within [start, start+length], pick it.
			// If at split point (end of one, start of next), which one?
			// If we are at end of a wrapped line, we are visually at end of that line.
			// If we are at start of next wrapped line, we are visually at start.
			// They are the same logical index!
			// We need to decide based on "affinity".
			// But for ArrowUp, we just want "the visual line we are on".
			// If we are at split point, logically we are at index X.
			// Visually, index X is end of line A and start of line B.
			// Standard behavior: if I press End, I'm at end of A. If I type char, it stays on A (until wrap).
			// If I just arrived there, usually I am at B start if wrapped?
			// Let's assume strict inequality for start: caretColumn < start + length, except for last segment.

			if (vIdx == -1 && !visualLines.empty())
			{
				// fallback search
				for (size_t i = 0; i < visualLines.size(); ++i)
				{
					if (visualLines[i].logicalLineIndex == currentLine) vIdx = (i32)i; // last one
					if (visualLines[i].logicalLineIndex > currentLine) break;
				}
			}

			if (vIdx > 0)
			{
				const auto& currVl = visualLines[vIdx];
				const auto& prevVl = visualLines[vIdx - 1];

				i32 dist = caretColumn - currVl.startColumn;
				// target is min(dist, prevVl.length) relative to start

				currentLine = prevVl.logicalLineIndex;
				caretColumn = prevVl.startColumn + std::min(dist, prevVl.length);
			}
		}

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = prevLine;
				selectionStartColumn = prevColumn;
			}

			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
		{
			deselect();
		}
	}
	else if (ev.key.code == KeyCode::Home)
	{
		caretPreferLineEnd = false;
		i32 prevLine = currentLine;
		i32 prevColumn = caretColumn;

		// Move to start of visual line
		if (!visualLines.empty())
		{
			// Find current visual line
			for (size_t i = 0; i < visualLines.size(); ++i)
			{
				const auto& vl = visualLines[i];
				if (vl.logicalLineIndex == currentLine)
				{
					bool isLastSeg = (i + 1 >= visualLines.size() || visualLines[i + 1].logicalLineIndex != currentLine);

					bool match = false;
					if (isLastSeg)
						match = (caretColumn >= vl.startColumn && caretColumn <= vl.startColumn + vl.length);
					else
						match = (caretColumn >= vl.startColumn && caretColumn < vl.startColumn + vl.length);

					if (match)
					{
						// Found it
						caretColumn = vl.startColumn;
						break;
					}
				}
			}
		}
		else
		{
			caretColumn = 0;
		}

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = prevLine;
				selectionStartColumn = prevColumn;
			}
			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
		{
			deselect();
		}
	}
	else if (ev.key.code == KeyCode::End)
	{
		caretPreferLineEnd = true;
		i32 prevLine = currentLine;
		i32 prevColumn = caretColumn;

		// Move to end of visual line
		if (!visualLines.empty())
		{
			// Find current visual line
			for (size_t i = 0; i < visualLines.size(); ++i)
			{
				const auto& vl = visualLines[i];
				if (vl.logicalLineIndex == currentLine)
				{
					bool isLastSeg = (i + 1 >= visualLines.size() || visualLines[i + 1].logicalLineIndex != currentLine);

					bool match = false;
					if (isLastSeg)
						match = (caretColumn >= vl.startColumn && caretColumn <= vl.startColumn + vl.length);
					else
						match = (caretColumn >= vl.startColumn && caretColumn < vl.startColumn + vl.length);

					if (match)
					{
						// Found it
						caretColumn = vl.startColumn + vl.length;
						break;
					}
				}
			}
		}
		else
		{
			caretColumn = lines[currentLine].size();
		}

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = prevLine;
				selectionStartColumn = prevColumn;
			}
			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
		{
			deselect();
		}
	}
	else if (ev.key.code == KeyCode::ArrowDown)
	{
		i32 prevLine = currentLine;
		i32 prevColumn = caretColumn;

		// Move visually down
		if (!visualLines.empty())
		{
			i32 vIdx = -1;
			// Find current visual line
			for (size_t i = 0; i < visualLines.size(); ++i)
			{
				const auto& vl = visualLines[i];
				if (vl.logicalLineIndex == currentLine)
				{
					// logic to find correct visual segment:
					// if caret < start + length, found.
					// if caret == start + length, it could be this one (if last) or next one (if wrapped).
					// usually cursor stays on previous line if possible? No, it flows.
					// Let's bias towards the START of the next line if ambiguous?
					// No, bias towards END of current line if ambiguous (e.g. typing)
					// But for navigation, we usually want stability.

					bool isLastSeg = (i + 1 >= visualLines.size() || visualLines[i+1].logicalLineIndex != currentLine);

					if (caretColumn >= vl.startColumn && caretColumn < vl.startColumn + vl.length)
					{
						vIdx = (i32)i;
						break;
					}

					bool isLastSegment = (i + 1 >= visualLines.size() || visualLines[i+1].logicalLineIndex != currentLine);
					if (caretColumn == vl.startColumn + vl.length)
					{
						if (isLastSegment || caretPreferLineEnd)
						{
							vIdx = (i32)i;
							break;
						}
					}
				}
			}

			// Reset affinity after choice
			caretPreferLineEnd = false;

			// Fallback
			if (vIdx == -1)
			{
				for (size_t i = 0; i < visualLines.size(); ++i)
					if (visualLines[i].logicalLineIndex == currentLine) vIdx = (i32)i;
			}

			if (vIdx != -1 && vIdx < (i32)visualLines.size() - 1)
			{
				const auto& currVl = visualLines[vIdx];
				const auto& nextVl = visualLines[vIdx + 1];

				i32 dist = caretColumn - currVl.startColumn;

				currentLine = nextVl.logicalLineIndex;
				caretColumn = nextVl.startColumn + std::min(dist, nextVl.length);
			}
		}

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = prevLine;
				selectionStartColumn = prevColumn;
			}

			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
		{
			deselect();
		}
	}

	else if (ev.key.code == KeyCode::Tab)
	{
		// use insertTextAtCaret to handle selection deletion etc
		Utf32String tabStr;
		tabStr.push_back('\t');
		insertTextAtCaret(tabStr);
	}
	else if (ev.key.code == KeyCode::Enter)
	{
		// debounce Enter key to prevent double insertion from same-frame events
		if (lastKeyProcessFrame == ctx->frameCount)
			return;

		lastKeyProcessFrame = ctx->frameCount;

		// use insertTextAtCaret to handle selection deletion and consistent newline insertion
		Utf32String newline;
		newline.push_back('\n');
		insertTextAtCaret(newline);
	}
	else if (ev.key.code == KeyCode::Backspace)
	{
		if (selectionActive)
		{
			deleteSelection();
		}
		else if (caretColumn > 0)
		{
			lines[currentLine].erase(lines[currentLine].begin() + caretColumn - 1);
			caretColumn--;
			textChanged = true;
		}
		else if (currentLine > 0)
		{
			// merge with previous line
			caretColumn = lines[currentLine - 1].size();
			lines[currentLine - 1].insert(lines[currentLine - 1].end(), lines[currentLine].begin(), lines[currentLine].end());
			lines.erase(lines.begin() + currentLine);
			currentLine--;
			textChanged = true;
		}

		// soft scroll up if we have empty space at bottom
		if (textChanged)
		{
			f32 lineHeight = font ? font->getMetrics().height : 20.0f;
			f32 contentHeight = lines.size() * lineHeight;
			f32 maxScrollY = std::max(0.0f, contentHeight - clipRect.height);
			if (scrollOffsetY > maxScrollY)
				scrollOffsetY = std::max(maxScrollY, scrollOffsetY - lineHeight);
		}
	}
	else if (ev.key.code == KeyCode::Delete)
	{
		if (selectionActive)
		{
			deleteSelection();
		}
		else if (caretColumn < lines[currentLine].size())
		{
			lines[currentLine].erase(lines[currentLine].begin() + caretColumn);
			textChanged = true;
		}
		else if (currentLine < lines.size() - 1)
		{
			// merge with next line
			lines[currentLine].insert(lines[currentLine].end(), lines[currentLine + 1].begin(), lines[currentLine + 1].end());
			lines.erase(lines.begin() + currentLine + 1);
			textChanged = true;
		}

		// soft scroll up if we have empty space at bottom
		if (textChanged)
		{
			f32 lineHeight = font ? font->getMetrics().height : 20.0f;
			f32 contentHeight = lines.size() * lineHeight;
			f32 maxScrollY = std::max(0.0f, contentHeight - clipRect.height);
			if (scrollOffsetY > maxScrollY)
				scrollOffsetY = std::max(maxScrollY, scrollOffsetY - lineHeight);
		}
	}
	else if (ev.key.code == KeyCode::Home)
	{
		// calculate indentation (first non-whitespace char)
		i32 indentation = 0;
		if (currentLine < lines.size())
		{
			const auto& line = lines[currentLine];
			for (size_t i = 0; i < line.size(); i++)
			{
				if (line[i] != ' ' && line[i] != '\t')
				{
					indentation = (i32)i;
					break;
				}
			}
		}

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = currentLine;
				selectionStartColumn = caretColumn;
			}

			// toggle behavior: if at 0, go to indentation. if at indentation, go to 0. otherwise go to indentation first?
			// User request: "should select only to line start, and on next presses should toggle select the indenting..."
			// implying: standard is 0. then indent.
			// VS Code behavior: Home goes to indent first, then 0.
			// Let's implement: if caret is at 0, go to indent. if caret is at indent, go to 0. if caret is elsewhere, go to indent (or 0?).
			// strict reading of user request: "select only to line start [0], and on next presses toggle... [indent]"

			if (caretColumn == 0)
				caretColumn = indentation;
			else
				caretColumn = 0;

			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
		{
			if (caretColumn == 0)
				caretColumn = indentation;
			else
				caretColumn = 0;

			deselect();
		}
	}
	else if (ev.key.code == KeyCode::End)
	{
		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = currentLine;
				selectionStartColumn = caretColumn;
			}

			caretColumn = lines[currentLine].size();

			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
		{
			caretColumn = lines[currentLine].size();
			deselect();
		}
	}
	else if (ev.key.code == KeyCode::PgUp)
	{
		i32 prevLine = currentLine;
		i32 prevColumn = caretColumn;

		currentLine -= visibleLineCount;
		if (currentLine < 0)
		{
			currentLine = 0;
			caretColumn = 0;
		}
		else if (caretColumn > lines[currentLine].size())
		{
			caretColumn = lines[currentLine].size();
		}

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = prevLine;
				selectionStartColumn = prevColumn;
			}
			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
		{
			deselect();
		}
	}
	else if (ev.key.code == KeyCode::PgDown)
	{
		i32 prevLine = currentLine;
		i32 prevColumn = caretColumn;

		currentLine += visibleLineCount;
		if (currentLine >= lines.size())
		{
			currentLine = lines.size() - 1;
			caretColumn = lines[currentLine].size();
		}
		else if (caretColumn > lines[currentLine].size())
		{
			caretColumn = lines[currentLine].size();
		}

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = prevLine;
				selectionStartColumn = prevColumn;
			}
			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
		{
			deselect();
		}
	}
	else if (ev.key.code == KeyCode::A && has(ev.key.modifiers, KeyModifiers::Control))
	{
		selectAll();
	}
	else if (ev.key.code == KeyCode::C && has(ev.key.modifiers, KeyModifiers::Control))
	{
		Utf32String str = getSelection();
		if (!str.empty())
		{
			char* tmpStr = nullptr;
			ctx->settings.services.utf32To8(str, &tmpStr);
			copyToClipboard(tmpStr);
			delete[] tmpStr;
		}
	}
	else if (ev.key.code == KeyCode::V && has(ev.key.modifiers, KeyModifiers::Control))
	{
		if (selectionActive)
			deleteSelection();

		static const u32 maxTextSize = 8192;
		char tmpStr[maxTextSize];
		Utf32String utf32Str;

		pasteFromClipboard(tmpStr, maxTextSize);

		if (ctx->settings.services.utf8To32(tmpStr, utf32Str))
		{
			insertTextAtCaret(utf32Str);
			textChanged = true;
		}
	}
	else if (ev.key.code == KeyCode::X && has(ev.key.modifiers, KeyModifiers::Control))
	{
		Utf32String str = getSelection();

		if (selectionActive && !str.empty())
		{
			deleteSelection();

			char* str8 = nullptr;
			if (ctx->settings.services.utf32To8(str, &str8))
			{
				copyToClipboard(str8);
				delete[] str8;
				textChanged = true;
			}
		}
	}

	ensureCaretVisible();
}

}
