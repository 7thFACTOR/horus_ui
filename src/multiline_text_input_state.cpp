#include "multiline_text_input_state.h"
#include "util.h"
#include "font.h"
#include "context.h"
#include "theme.h"
#include <chrono>
#include <algorithm>

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

	// Calculate deleted length BEFORE modifying lines
	size_t deletedCount = 0;
	if (startLine == endLine)
	{
		deletedCount = endCol - startCol;
	}
	else
	{
		deletedCount = (lines[startLine].size() - startCol) + endCol + (endLine - startLine);
		for (i32 i = startLine + 1; i < endLine; i++)
			deletedCount += lines[i].size();
	}

	if (deletedCount > totalTextLength) totalTextLength = 0;
	else totalTextLength -= deletedCount;

	if (startLine == endLine)
	{
		// single line deletion
		lines[startLine].erase(lines[startLine].begin() + startCol, lines[startLine].begin() + endCol);
		markLineDirty(startLine);
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

		// Synchronize other per-line caches
		if (lineVisuals.size() > (size_t)endLine)
		{
			lineVisuals.erase(lineVisuals.begin() + startLine, lineVisuals.begin() + endLine + 1);
			lineVisuals.insert(lineVisuals.begin() + startLine, std::vector<VisualLine>());
		}
		if (lineStates.size() > (size_t)endLine)
		{
			lineStates.erase(lineStates.begin() + startLine, lineStates.begin() + endLine + 1);
			lineStates.insert(lineStates.begin() + startLine, -1);
		}

		markLineDirty(startLine);
	}

	// Prevent stale pointers: visualLines holds pointers into lineVisuals; clear and force recompute.
	visualLines.clear();
	lineVisuals.resize(lines.size());
	if (lineStates.size() != lines.size()) lineStates.resize(lines.size(), -1);
	caretVisualLineIndex = (size_t)-1;
	firstDirtyLine = 0;
	forceLayoutUpdate = true;

	currentLine = startLine;
	caretColumn = startCol;
	caretVisualLineIndex = (size_t)-1;
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
	totalTextLength = 0;
	markLineDirty(0);
}

void MultilineTextInputState::insertTextAtCaret(const Utf32String& newText)
{
	if (selectionActive)
		deleteSelection();

	caretVisualLineIndex = (size_t)-1;
	markLineDirty(currentLine);

	if (newText.empty())
		return;

	if (totalTextLength >= maxTextLength)
		return;

	// handle newlines in pasted text
	for (u32 ch : newText)
	{
		// check limit
		if (totalTextLength >= maxTextLength)
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
			if (lineVisuals.size() > (size_t)currentLine - 1)
				lineVisuals.insert(lineVisuals.begin() + currentLine, std::vector<VisualLine>());
			if (lineStates.size() > (size_t)currentLine - 1)
				lineStates.insert(lineStates.begin() + currentLine, -1);
			caretColumn = 0;
			totalTextLength++;
		}
		else if (ch == '\t')
		{
			if (has(flags, MultilineTextInputFlags::SpacesOnTab))
			{
				u32 tabSize = ctx->settings.tabSize;

				// expand tab to N spaces
				for (u32 k = 0; k < tabSize; k++)
				{
					if (totalTextLength >= maxTextLength)
						break;

					lines[currentLine].insert(lines[currentLine].begin() + caretColumn, ' ');
					caretColumn++;
					totalTextLength++;
				}
			}
			else
			{
				lines[currentLine].insert(lines[currentLine].begin() + caretColumn, '\t');
				caretColumn++;
				totalTextLength++;
			}
		}
		else
		{
			lines[currentLine].insert(lines[currentLine].begin() + caretColumn, ch);
			caretColumn++;
			totalTextLength++;
		}
	}

	// Prevent stale pointers: visualLines holds pointers into lineVisuals; clear and force recompute.
	visualLines.clear();
	lineVisuals.resize(lines.size());
	if (lineStates.size() != lines.size()) lineStates.resize(lines.size(), -1);
	caretVisualLineIndex = (size_t)-1;
	firstDirtyLine = 0;
	forceLayoutUpdate = true;

	textChanged = true;
}

Point MultilineTextInputState::getCaretScreenPosition()
{
	auto& elemState = themeElement->normalState();
	Font* font = elemState.font;

	if (!font || lines.empty())
		return Point(clipRect.x, clipRect.y);

	f32 lineHeight = font->getMetrics().height;

	size_t foundVisualLine = caretVisualLineIndex;

	if (foundVisualLine == (size_t)-1 || foundVisualLine >= visualLines.size()
		|| visualLines[foundVisualLine]->logicalLineIndex != currentLine)
	{
		// Fallback: search if cache is invalid using Binary Search (O(log N))
		auto it = std::lower_bound(visualLines.begin(), visualLines.end(), currentLine, 
			[](const VisualLine* vl, i32 lineIdx) {
				return vl->logicalLineIndex < lineIdx;
			});

		if (it != visualLines.end() && (*it)->logicalLineIndex == currentLine)
		{
			size_t startIdx = std::distance(visualLines.begin(), it);
			foundVisualLine = startIdx;

			// Check if we need to refine for wrapped lines
			for (size_t i = startIdx; i < visualLines.size() && visualLines[i]->logicalLineIndex == currentLine; ++i)
			{
				const auto vl = visualLines[i];
				bool isLastSegment = true;
				if (i + 1 < visualLines.size() && visualLines[i + 1]->logicalLineIndex == currentLine)
					isLastSegment = false;

				if (caretColumn >= vl->startColumn && caretColumn < vl->startColumn + vl->length)
				{
					foundVisualLine = i;
					break;
				}

				if (caretColumn == vl->startColumn + vl->length)
				{
					if (isLastSegment || caretPreferLineEnd)
					{
						foundVisualLine = i;
						break;
					}
				}
			}
		}
	}

	if (foundVisualLine == (size_t)-1 && !visualLines.empty())
	{
		if (currentLine >= (i32)lines.size()) foundVisualLine = visualLines.size() - 1;
		else
		{
			for (i32 i = (i32)visualLines.size() - 1; i >= 0; i--)
			{
				if (visualLines[i]->logicalLineIndex == currentLine)
				{
					foundVisualLine = (size_t)i;
					break;
				}
			}
		}
	}

	if (foundVisualLine == (size_t)-1)
		return Point(clipRect.x - scrollOffsetX, clipRect.y - scrollOffsetY);

	const auto vl = visualLines[foundVisualLine];
	
	i32 relCaret = caretColumn - vl->startColumn;
	if (relCaret < 0) relCaret = 0;
	if (relCaret > vl->length) relCaret = vl->length;

	// Use robust measurement that matches renderer logic (tokens)
	f32 xOffset = calculateTextSegmentWidth(font, lines[currentLine], vl->startColumn, relCaret, vl->logicalLineIndex);

	return Point(
		clipRect.x + xOffset - scrollOffsetX,
		clipRect.y + (f32)foundVisualLine * lineHeight - scrollOffsetY
	);
}

f32 MultilineTextInputState::calculateTextSegmentWidth(Font* font, const Utf32String& line, i32 startCol, i32 length, i32 logicalLineIndex)
{
	if (length <= 0) return 0.0f;
	if (!font) return 0.0f;

	// Determine starting state
	i32 currentState = -1;
	if (lineStates.size() > logicalLineIndex && logicalLineIndex >= 0)
		currentState = lineStates[logicalLineIndex];

	// Fast-forward state if needed
	if (startCol > 0 && !rules32.empty())
	{
		// Scan from 0 to startCol
		for (i32 c = 0; c < startCol; )
		{
			if (currentState != -1)
			{
				const auto& endKw = rules32[currentState].end;
				if (endKw.empty()) { currentState = -1; break; } // EOL

				bool match = true;
				if (c + endKw.size() > startCol) { 
					// Match crosses boundary? We assume state persists until end of match?
					// Ideally we scan char by char.
					// If match starts before startCol, and ends after/at startCol, we exit state at endKw.
					// But we only care about state AT startCol.
					// If match ends after startCol, we are technically "inside" the rule until match ends?
					// Or does rule end AT start of endKw?
					// Renderer: `c += endKw.size(); currentState = -1;`
					// So until we consume endKw, we are in state.
				}
				
				// Check match at c
				if (c + endKw.size() <= line.size())
				{
					for (size_t k = 0; k < endKw.size(); k++)
						if (line[c + k] != endKw[k]) { match = false; break; }
				} else match = false;

				if (match)
				{
					// Check escape
					bool escaped = false;
					const auto& escKw = rules32[currentState].escape;
					if (!escKw.empty())
					{
						size_t escCount = 0;
						size_t backIdx = c;
						while (backIdx >= escKw.size())
						{
							backIdx -= escKw.size();
							bool escMatch = true;
							for (size_t k = 0; k < escKw.size(); k++)
								if (line[backIdx+k] != escKw[k]) { escMatch = false; break; }
							if (escMatch) escCount++; else break;
						}
						if (escCount % 2 != 0) escaped = true;
					}

					if (!escaped)
					{
						c += endKw.size();
						currentState = -1;
						continue;
					}
				}
				c++;
			}
			else
			{
				i32 bestRule = -1;
				// Check rule starts
				for (u32 r = 0; r < rules32.size(); r++)
				{
					const auto& startKw = rules32[r].begin;
					if (startKw.empty()) continue;
					if (c + startKw.size() > line.size()) continue;

					bool match = true;
					for (size_t k = 0; k < startKw.size(); k++)
						if (line[c + k] != startKw[k]) { match = false; break; }

					if (match)
					{
						bestRule = (i32)r;
						break; // Priority
					}
				}

				if (bestRule != -1)
				{
					currentState = bestRule;
					c += rules32[bestRule].begin.size();
				}
				else
				{
					c++;
				}
			}
		}
	}

	// Now measure from startCol to startCol + length
	f32 totalWidth = 0.0f;
	i32 endCol = startCol + length;
	if (endCol > line.size()) endCol = (i32)line.size();

	for (i32 c = startCol; c < endCol; )
	{
		size_t segStart = c;
		size_t segEnd = endCol;
		bool advanceState = false;
		i32 nextState = currentState;

		if (currentState != -1)
		{
			const auto& endKw = rules32[currentState].end;
			if (endKw.empty())
			{
				segEnd = endCol;
				// nextState = -1; // Effectively EOL
			}
			else
			{
				// Search for endKw
				size_t matchPos = std::string::npos;
				for (size_t i = c; i + endKw.size() <= endCol; i++) // only search up to endCol? No, rule can end AFTER visible area?
				// But we only measure up to endCol.
				// If rule ends AFTER endCol, then [c, endCol) is all inside rule.
				// But what if rule ends EXACTLY at range boundary?
				// We need to find match even if it crosses boundary?
				// No, we only measure text UP TO endCol.
				// If match starts at endCol, it's outside.
				// If match starts before endCol, we should split there.
				{
					bool found = true;
					for (size_t k = 0; k < endKw.size(); k++)
						if (line[i+k] != endKw[k]) { found = false; break; }
					
					if (found)
					{
						// Check escape
						bool escaped = false;
						const auto& escKw = rules32[currentState].escape;
						if (!escKw.empty())
						{
							size_t escCount = 0;
							size_t backIdx = i;
							while (backIdx >= escKw.size())
							{
								backIdx -= escKw.size();
								bool escMatch = true;
								for (size_t k = 0; k < escKw.size(); k++)
									if (line[backIdx+k] != escKw[k]) { escMatch = false; break; }
								if (escMatch) escCount++; else break;
							}
							if (escCount % 2 != 0) escaped = true;
						}

						if (!escaped)
						{
							matchPos = i;
							break;
						}
					}
				}

				if (matchPos != std::string::npos)
				{
					segEnd = matchPos + endKw.size();
					if (segEnd > endCol) segEnd = endCol; // Clamp if endKw crosses boundary?
					// Actually if matchPos < endCol, but matchPos + len > endCol.
					// We measure up to endCol.
					// But logic says we consume the token.
					// If we are measuring visual WIDTH.
					// We should measure [c, matchPos) then [matchPos, matchPos+len).
					// But syntax highlighting changes color, not necessarily font.
					// However, splitting breaks kerning.
					// So yes, we must process the split.
					if (matchPos < endCol)
					{
						segEnd = matchPos + endKw.size(); // This might exceed endCol, handled by measure clamp?
						nextState = -1;
					}
				}
			}
		}
		else
		{
			// Check Keywords/Ranges
			size_t bestPos = std::string::npos;
			i32 foundRule = -1;
			const Keyword32* foundKw = nullptr;

			for (u32 r = 0; r < rules32.size(); r++)
			{
				const auto& startKw = rules32[r].begin;
				if (startKw.empty()) continue;
				// Search limit is endCol?
				// If match starts before endCol.
				size_t limit = (bestPos == std::string::npos) ? endCol : bestPos;
				if (c + startKw.size() > line.size()) continue;

				for (size_t i = c; i + startKw.size() <= limit && i + startKw.size() <= line.size(); i++)
				{
					bool match = true;
					for (size_t k = 0; k < startKw.size(); k++)
						if (line[i+k] != startKw[k]) { match = false; break; }
					
					if (match)
					{
						bestPos = i;
						foundRule = (i32)r;
						foundKw = nullptr;
						limit = bestPos;
						break;
					}
				}
			}

			if (!keywords32.empty())
			{
				size_t limit = (bestPos == std::string::npos) ? endCol : bestPos;
				
				for (u32 k = 0; k < keywords32.size(); k++)
				{
					const auto& kwStr = keywords32[k].keyword;
					if (c + kwStr.size() > limit) continue;

					for (size_t i = c; i + kwStr.size() <= limit && i + kwStr.size() <= line.size(); i++)
					{
                        if (i == bestPos && foundRule != -1) break; 
						bool match = true;
						for (size_t sub = 0; sub < kwStr.size(); sub++)
							if (line[i+sub] != kwStr[sub]) { match = false; break; }
						
						if (match)
						{
                            if (i < bestPos || bestPos == std::string::npos)
                            {
                                bestPos = i;
                                foundRule = -1;
                                foundKw = &keywords32[k];
                                limit = bestPos;
                            }
							break;
						}
					}
				}
			}

			if (bestPos != std::string::npos && bestPos < endCol)
			{
				if (bestPos > c)
				{
					segEnd = bestPos; // Draw normal text up to match
					// Next iter will handle match
				}
				else
				{
					// At match
					if (foundRule != -1)
					{
						currentState = foundRule;
						continue; // Re-eval loop to handle rule inside
					}
					else if (foundKw)
					{
						segEnd = c + foundKw->keyword.size();
						// nextState -1
					}
				}
			}
		}

		// Measure [segStart, segEnd)
		// Clamp to endCol (we only want width up to caret)
		size_t measureEnd = segEnd;
		if (measureEnd > (size_t)endCol) measureEnd = (size_t)endCol;
		
		if (measureEnd > segStart)
		{
			totalWidth += font->computeTextSize(line.data() + segStart, (u32)(measureEnd - segStart)).width;
		}

		c = (i32)segEnd; // Advance to next segment (might be past endCol if token crossed, but checking loop condition)
		currentState = nextState;
	}

	return totalWidth;
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
			computeVisualLines(font, lastLayoutWidth, nullptr, 0, nullptr, 0);
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

	const auto vl = visualLines[visualLineIdx];
	currentLine = vl->logicalLineIndex;

	// find column within this visual segment
	f32 adjustedX = pt.x + scrollOffsetX;

	// The text on this visual line is a substring of logical line
	const auto& lineText = lines[currentLine];

	// Optimization: Only measure the substring for this visual line
	Utf32String segmentText(lineText.begin() + vl->startColumn, lineText.begin() + vl->startColumn + vl->length);

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
			caretColumn = vl->startColumn + relCol;
			return caretColumn;
		}
	}

	// if past end of visual line's content, place at end of visual line
	caretColumn = vl->startColumn + vl->length;
	caretVisualLineIndex = (size_t)visualLineIdx; // Update cache immediately on click!
	return caretColumn;
}

void MultilineTextInputState::computeVisualLines(Font* font, f32 availableWidth, const RangeHighlight* rules, u32 ruleCount, const KeywordInfo* keywords, u32 keywordCount)
{
	if (!font || lines.empty())
	{
		visualLines.clear();
		lineVisuals.clear();
		maxLineWidth = 0.0f;
		caretVisualLineIndex = (size_t)-1;
		return;
	}

	bool widthChanged = std::abs(lastLayoutWidth - availableWidth) > 0.1f;
	if (widthChanged || lineVisuals.empty() || forceLayoutUpdate)
	{
		lineVisuals.clear();
		lineVisuals.resize(lines.size());
		firstDirtyLine = 0;
		lastLayoutWidth = availableWidth;
	}

	if (lineVisuals.size() != lines.size())
	{
		lineVisuals.resize(lines.size());
		// markLineDirty should have been called by mutation methods
	}

	if (firstDirtyLine == -1 && !visualLines.empty() && !forceLayoutUpdate)
		return;

	printf("MultilineTextInputState::computeVisualLines entering: firstDirtyLine=%d, visualLines.empty=%d, forceLayoutUpdate=%d, textChanged=%d\n", 
		firstDirtyLine, (int)visualLines.empty(), (int)forceLayoutUpdate, (int)textChanged);

	auto total_layout_start = std::chrono::high_resolution_clock::now();
	i32 lines_processed = 0;

	i32 start = (firstDirtyLine == -1) ? 0 : firstDirtyLine;
	bool isWrapping = has(flags, MultilineTextInputFlags::WordWrap);

	for (size_t i = start; i < lines.size(); ++i)
	{
		lines_processed++;
		const Utf32String& line = lines[i];
		auto& visuals = lineVisuals[i];
		
		// For stable stop, keep track of what we had
		std::vector<VisualLine> oldVisuals = visuals;
		visuals.clear();

		if (!isWrapping)
		{
			VisualLine vl;
			vl.logicalLineIndex = (i32)i;
			vl.startColumn = 0;
			vl.length = (i32)line.size();
			vl.width = font->computeTextSize(line.data(), (u32)line.size()).width;
			visuals.push_back(vl);
		}
		else if (line.empty())
		{
			VisualLine vl;
			vl.logicalLineIndex = (i32)i;
			vl.startColumn = 0;
			vl.length = 0;
			vl.width = 0;
			visuals.push_back(vl);
		}
		else
		{
			i32 currentStart = 0;
			while (currentStart < (i32)line.size())
			{
				i32 remaining = (i32)line.size() - currentStart;
				i32 low = 1;
				i32 high = remaining;
				i32 fitLength = 0;

				while (low <= high)
				{
					i32 mid = low + (high - low) / 2;
					f32 w = font->computeTextSize(line.data() + currentStart, (u32)mid).width;
					if (w <= availableWidth)
					{
						fitLength = mid;
						low = mid + 1;
					}
					else high = mid - 1;
				}

				if (fitLength == 0 && remaining > 0) fitLength = 1;

				i32 lengthOnLine = fitLength;
				if (currentStart + fitLength < (i32)line.size())
				{
					i32 breakIdx = fitLength;
					bool foundBreak = false;
					for (i32 k = fitLength; k > 0; k--)
					{
						u32 ch = line[currentStart + k - 1];
						if (ch == ' ' || ch == '\t' || ch == '-') { breakIdx = k; foundBreak = true; break; }
					}
					if (foundBreak) lengthOnLine = breakIdx;
				}

				VisualLine vl;
				vl.logicalLineIndex = (i32)i;
				vl.startColumn = currentStart;
				vl.length = lengthOnLine;
				vl.width = font->computeTextSize(line.data() + currentStart, (u32)lengthOnLine).width;
				visuals.push_back(vl);

				currentStart += lengthOnLine;
			}
		}

		// Calculate segments for the logical line
		tempSegments.clear();
		i32 initialState = (i < (size_t)lineStates.size()) ? lineStates[i] : -1;
		calculateSegments(line, initialState, tempSegments, rules, ruleCount, keywords, keywordCount);

		// Distribute segments across visual lines
		for (auto& vl : visuals)
		{
			vl.segments.clear();
			i32 vlEnd = vl.startColumn + vl.length;
			i32 currentPos = 0;

			for (const auto& seg : tempSegments)
			{
				i32 segEnd = currentPos + seg.length;

				// Does this segment overlap with the visual line?
				i32 overlapStart = std::max(vl.startColumn, currentPos);
				i32 overlapEnd = std::min(vlEnd, segEnd);

				if (overlapStart < overlapEnd)
				{
					VisualSegment vseg;
					vseg.length = overlapEnd - overlapStart;
					vseg.color = seg.color;
					vl.segments.push_back(vseg);
				}

				currentPos = segEnd;
				if (currentPos >= vlEnd) break;
			}
		}

		// Stable stop check
		if (firstDirtyLine != -1 && (i32)i > firstDirtyLine && visuals.size() == oldVisuals.size())
		{
			bool match = true;
			for (size_t v = 0; v < visuals.size(); v++)
			{
				if (visuals[v].startColumn != oldVisuals[v].startColumn ||
					visuals[v].length != oldVisuals[v].length ||
					std::abs(visuals[v].width - oldVisuals[v].width) > 0.01f ||
					visuals[v].segments.size() != oldVisuals[v].segments.size())
				{
					match = false;
					break;
				}
				// check segments
				if (v < oldVisuals.size() && visuals[v].segments.size() == oldVisuals[v].segments.size())
				{
					for (size_t s = 0; s < visuals[v].segments.size(); s++)
					{
						if (visuals[v].segments[s].length != oldVisuals[v].segments[s].length ||
							visuals[v].segments[s].color.getRgba() != oldVisuals[v].segments[s].color.getRgba())
						{
							match = false;
							break;
						}
					}
				}
				else
				{
					match = false;
				}
				if (!match) break;
			}

			if (match)
			{
				// Stable! We can stop here.
				printf("computeVisualLines: stable stop at line %d (lines left: %zu)\n", (i32)i, lines.size() - i - 1);
				break;
			}
		}
	}

	if (lines_processed > 0 || visualLines.empty())
	{
		// Flatten results
		visualLines.clear();
		maxLineWidth = 0.0f;
		// DO NOT reset caretVisualLineIndex to -1 here if we are returning early or not re-calculating!
		// It should persist if the layout is stable.
		caretVisualLineIndex = (size_t)-1;

		// Pre-allocation to avoid re-allocs
		size_t estimatedTotal = lines.size();
		if (isWrapping) estimatedTotal = (size_t)(estimatedTotal * 1.2f);
		visualLines.reserve(estimatedTotal);

		for (size_t i = 0; i < lineVisuals.size(); ++i)
		{
			for (const auto& vl : lineVisuals[i])
			{
				visualLines.push_back(&vl); // Store pointers to avoid O(N) vector copies!
				if (vl.width > maxLineWidth) maxLineWidth = vl.width;

				if (vl.logicalLineIndex == currentLine)
				{
					if (caretColumn >= vl.startColumn && caretColumn <= vl.startColumn + vl.length)
					{
						if (caretVisualLineIndex == (size_t)-1 || caretColumn == vl.startColumn)
							caretVisualLineIndex = visualLines.size() - 1;
					}
				}
			}
		}
	}

	auto total_layout_end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> total_layout_elapsed = total_layout_end - total_layout_start;
	if (lines_processed > 0)
	{
		printf("computeVisualLines: processed %d lines in %.3f ms (avg %.4f/line, start: %d, reason: %s)\n", 
			lines_processed, total_layout_elapsed.count(), total_layout_elapsed.count() / lines_processed, 
			start, forceLayoutUpdate ? "force" : (visualLines.empty() ? "empty" : "dirty"));
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
			// Find visual index for caret - deterministic search
			for (size_t i = 0; i < visualLines.size(); ++i)
			{
				const auto vl = visualLines[i];
				if (vl->logicalLineIndex == currentLine)
				{
					bool isLastSegment = (i + 1 >= visualLines.size() || visualLines[i + 1]->logicalLineIndex != currentLine);

					if (caretColumn >= vl->startColumn && caretColumn < vl->startColumn + vl->length)
					{
						vIdx = (i32)i;
						break;
					}

					if (caretColumn == vl->startColumn + vl->length)
					{
						if (isLastSegment || caretPreferLineEnd)
						{
							vIdx = (i32)i;
							break;
						}
					}
				}
			}

			 // fallback to last visual segment for the current logical line
			if (vIdx == -1)
			{
				for (size_t i = 0; i < visualLines.size(); ++i)
				{
					if (visualLines[i]->logicalLineIndex == currentLine) vIdx = (i32)i;
					if (visualLines[i]->logicalLineIndex > currentLine) break;
				}
			}

			// Reset affinity after moving off the line
			caretPreferLineEnd = false;

			if (vIdx > 0)
			{
				const auto currVl = visualLines[vIdx];
				const auto prevVl = visualLines[vIdx - 1];

				i32 dist = caretColumn - currVl->startColumn;
				currentLine = prevVl->logicalLineIndex;
				caretColumn = prevVl->startColumn + std::min(dist, prevVl->length);

				// update cached visual index
				caretVisualLineIndex = (size_t)(vIdx - 1);
			}
		}

		// selection handling unchanged...
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
			// Find current visual index same deterministic way as ArrowUp
			for (size_t i = 0; i < visualLines.size(); ++i)
			{
				const auto vl = visualLines[i];
				if (vl->logicalLineIndex == currentLine)
				{
					bool isLastSegment = (i + 1 >= visualLines.size() || visualLines[i + 1]->logicalLineIndex != currentLine);

					if (caretColumn >= vl->startColumn && caretColumn < vl->startColumn + vl->length)
					{
						vIdx = (i32)i;
						break;
					}

					if (caretColumn == vl->startColumn + vl->length)
					{
						if (isLastSegment || caretPreferLineEnd)
						{
							vIdx = (i32)i;
							break;
						}
					}
				}
			}

			// Fallback
			if (vIdx == -1)
			{
				for (size_t i = 0; i < visualLines.size(); ++i)
					if (visualLines[i]->logicalLineIndex == currentLine) vIdx = (i32)i;
			}

			caretPreferLineEnd = false;

			if (vIdx != -1 && vIdx < (i32)visualLines.size() - 1)
			{
				const auto currVl = visualLines[vIdx];
				const auto nextVl = visualLines[vIdx + 1];

				i32 dist = caretColumn - currVl->startColumn;

				currentLine = nextVl->logicalLineIndex;
				caretColumn = nextVl->startColumn + std::min(dist, nextVl->length);

				// update cached visual index
				caretVisualLineIndex = (size_t)(vIdx + 1);
			}
		}

		// selection handling unchanged...
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
			markLineDirty(currentLine);
		}
		else if (currentLine > 0)
		{
			// merge with previous line
			caretColumn = lines[currentLine - 1].size();
			lines[currentLine - 1].insert(lines[currentLine - 1].end(), lines[currentLine].begin(), lines[currentLine].end());
			lines.erase(lines.begin() + currentLine);
			currentLine--;
			textChanged = true;
			markLineDirty(currentLine);

			// Prevent stale visual pointers after mutate
			visualLines.clear();
			lineVisuals.resize(lines.size());
			if (lineStates.size() != lines.size()) lineStates.resize(lines.size(), -1);
			caretVisualLineIndex = (size_t)-1;
			firstDirtyLine = 0;
			forceLayoutUpdate = true;
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
			markLineDirty(currentLine);
		}
		else if (currentLine < lines.size() - 1)
		{
			// merge with next line
			lines[currentLine].insert(lines[currentLine].end(), lines[currentLine + 1].begin(), lines[currentLine + 1].end());
			lines.erase(lines.begin() + currentLine + 1);
			textChanged = true;
			markLineDirty(currentLine);

			// Prevent stale visual pointers after mutate
			visualLines.clear();
			lineVisuals.resize(lines.size());
			if (lineStates.size() != lines.size()) lineStates.resize(lines.size(), -1);
			caretVisualLineIndex = (size_t)-1;
			firstDirtyLine = 0;
			forceLayoutUpdate = true;
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

// Helper to hash rules
static u64 computeRulesHash(const RangeHighlight* rules, u32 count, const KeywordInfo* keywords, u32 keywordCount)
{
	u64 h = 0x811c9dc5; // FNV offset basis
	auto hashStr = [&](const char* s) {
		if (!s) return;
		while (*s) {
			h ^= (u64)*s++;
			h *= 0x100000001b3; // FNV prime
		}
	};

	for (u32 i = 0; i < count; i++)
	{
		hashStr(rules[i].beginKeyword);
		hashStr(rules[i].endKeyword);
		hashStr(rules[i].escapeKeyword);
		h ^= rules[i].color.getRgba();
		h *= 0x100000001b3;
	}
	for (u32 i = 0; i < keywordCount; i++)
	{
		hashStr(keywords[i].keyword);
		h ^= keywords[i].color.getRgba();
		h *= 0x100000001b3;
	}
	return h;
}

void MultilineTextInputState::updateSyntaxHighlighting(const RangeHighlight* rules, u32 count, const KeywordInfo* keywords, u32 keywordCount)
{
	// O(1) fast-path check: if pointers and counts match and text is clean, skip everything
	if (!textChanged 
		&& rules == lastRulesPtr 
		&& count == lastRuleCount 
		&& keywords == lastKeywordsPtr 
		&& keywordCount == lastKeywordCount 
		&& lineStates.size() == lines.size())
		return;

	u64 newHash = computeRulesHash(rules, count, keywords, keywordCount);
	u64 prevHash = lastRulesHash; // save previous hash to decide incremental vs full

	// Update pointers/counters used for future fast-path checks (but don't use lastRulesHash yet)
	lastRulesPtr = rules;
	lastRuleCount = count;
	lastKeywordsPtr = keywords;
	lastKeywordCount = keywordCount;

	if (lineStates.size() != lines.size())
		lineStates.resize(lines.size(), -1);

	printf("MultilineTextInputState::updateSyntaxHighlighting entering: firstDirtyLine=%d, textChanged=%d\n", firstDirtyLine, (int)textChanged);
	auto total_syntax_start = std::chrono::high_resolution_clock::now();
	i32 syntax_lines_processed = 0;

	// Convert rules/keywords to UTF-32
	rules32.resize(count);
	for (u32 i = 0; i < count; i++)
	{
		ctx->settings.services.utf8To32(rules[i].beginKeyword, rules32[i].begin);
		ctx->settings.services.utf8To32(rules[i].endKeyword, rules32[i].end);
		if (rules[i].escapeKeyword)
			ctx->settings.services.utf8To32(rules[i].escapeKeyword, rules32[i].escape);
		rules32[i].info = &rules[i];
	}

	keywords32.resize(keywordCount);
	for (u32 i = 0; i < keywordCount; i++)
	{
		ctx->settings.services.utf8To32(keywords[i].keyword, keywords32[i].keyword);
		keywords32[i].info = &keywords[i];
	}

	if (lines.empty())
	{
		lastRulesHash = newHash;
		return;
	}

	i32 start = 0;
	i32 currentState = -1;

	// Decide incremental vs full rescan using previous hash (prevHash)
	if (prevHash == newHash && !lineStates.empty() && firstDirtyLine != -1)
	{
		// incremental: start one line earlier to preserve multi-line rule state
		start = std::max(0, firstDirtyLine - 1);
		// seed currentState from previous line if available
		if (start > 0)
			currentState = lineStates[start - 1];
		else
			currentState = -1;
	}
	else
	{
		// full rescan needed: reset states
		start = 0;
		currentState = -1;
		std::fill(lineStates.begin(), lineStates.end(), -1);
	}

	// Perform scan starting at 'start'
	for (size_t i = (size_t)start; i < lines.size(); ++i)
	{
		syntax_lines_processed++;
		lineStates[i] = currentState;
		const Utf32String& line = lines[i];

		for (size_t c = 0; c < line.size(); )
		{
			// If inside a rule, check for end
			if (currentState != -1)
			{
				const auto& endKw = rules32[currentState].end;

				// Handle empty end keyword as "end of line"
				if (endKw.empty())
				{
					currentState = -1;
					break;
				}

				bool match = true;
				if (c + endKw.size() > line.size()) match = false;
				else
				{
					for (size_t k = 0; k < endKw.size(); k++) if (line[c + k] != endKw[k]) { match = false; break; }

					if (match && !rules32[currentState].escape.empty())
					{
						// Count consecutive escapes ending at c-1
						size_t escCount = 0;
						size_t backIdx = c;
						while (backIdx >= rules32[currentState].escape.size())
						{
							backIdx -= rules32[currentState].escape.size();
							bool escMatch = true;
							for (size_t k = 0; k < rules32[currentState].escape.size(); k++)
								if (line[backIdx + k] != rules32[currentState].escape[k]) { escMatch = false; break; }
							if (escMatch) escCount++;
							else break;
						}
						if (escCount % 2 != 0)
							match = false;
					}
				}

				if (match)
				{
					c += endKw.size();
					currentState = -1;
				}
				else
				{
					c++;
				}
			}
			else
			{
				// Check for rule starts
				i32 bestRule = -1;
				for (u32 r = 0; r < count; r++)
				{
					const auto& startKw = rules32[r].begin;
					if (startKw.empty()) continue;

					bool match = true;
					if (c + startKw.size() > line.size()) match = false;
					else
					{
						for (size_t k = 0; k < startKw.size(); k++)
							if (line[c + k] != startKw[k]) { match = false; break; }
					}

					if (match)
					{
						bestRule = (i32)r;
						break;
					}
				}

				if (bestRule != -1)
				{
					currentState = bestRule;
					c += rules32[bestRule].begin.size();
				}
				else
				{
					c++;
				}
			}
		}

		// If a rule is still open and its end is single-line, close it here
		if (currentState != -1)
		{
			if (rules32[currentState].end.empty() || (rules32[currentState].end.size() == 1 && rules32[currentState].end[0] == '\n'))
			{
				currentState = -1;
			}
		}

		// Incremental stop: if we've passed the dirty region and state matches next line, stop
		if (firstDirtyLine != -1 && (i32)i > firstDirtyLine && (i32)i + 1 < (i32)lineStates.size())
		{
			if (lineStates[i + 1] == currentState)
			{
				break;
			}
		}
	}

	// commit the new rules hash after processing
	lastRulesHash = newHash;

	auto total_syntax_end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> total_syntax_elapsed = total_syntax_end - total_syntax_start;
	if (syntax_lines_processed > 0)
	{
		printf("updateSyntaxHighlighting: processed %d lines in %.3f ms (avg %.4f ms/line)\n",
			syntax_lines_processed, total_syntax_elapsed.count(), total_syntax_elapsed.count() / syntax_lines_processed);
	}
}

void MultilineTextInputState::markLineDirty(i32 logicalLineIndex)
{
	if (logicalLineIndex < 0) return;
	if (firstDirtyLine == -1 || logicalLineIndex < firstDirtyLine)
		firstDirtyLine = logicalLineIndex;
}

void MultilineTextInputState::calculateSegments(const Utf32String& line, i32 initialState, std::vector<VisualSegment>& outSegments,
	const RangeHighlight* rules, u32 ruleCount, const KeywordInfo* keywords, u32 keywordCount)
{
	outSegments.clear();
	if (line.empty()) return;
	auto start_time = std::chrono::high_resolution_clock::now();

	i32 currentState = initialState;
	i32 lastSwitchPos = 0;
	Color defaultColor = themeElement->normalState().textColor;

	// Helper to add segment
	auto appendSegment = [&](i32 end, Color color) {
		if (end > lastSwitchPos)
		{
			VisualSegment seg;
			seg.length = end - lastSwitchPos;
			seg.color = color;
			outSegments.push_back(seg);
			lastSwitchPos = end;
		}
	};

	for (size_t c = 0; c < line.size(); )
	{
		i32 oldState = currentState;
		if (currentState != -1)
		{
			const auto& endKw = rules32[currentState].end;
			if (endKw.empty()) { currentState = -1; appendSegment((i32)line.size(), rules32[oldState].info->color); break; }

			bool match = true;
			if (c + endKw.size() > line.size()) match = false;
			else
			{
				for (size_t k = 0; k < endKw.size(); k++) if (line[c + k] != endKw[k]) { match = false; break; }
				if (match && !rules32[currentState].escape.empty())
				{
					size_t escCount = 0; i32 backIdx = (i32)c;
					while (backIdx >= (i32)rules32[currentState].escape.size())
					{
						backIdx -= (i32)rules32[currentState].escape.size();
						bool escMatch = true;
						for (size_t k = 0; k < rules32[currentState].escape.size(); k++) if (line[backIdx + k] != rules32[currentState].escape[k]) { escMatch = false; break; }
						if (escMatch) escCount++; else break;
					}
					if (escCount % 2 != 0) match = false;
				}
			}

			if (match)
			{
				appendSegment((i32)c + (i32)endKw.size(), rules32[currentState].info->color);
				c += endKw.size();
				currentState = -1;
			}
			else c++;
		}
		else
		{
			// Try keywords first
			bool keywordMatched = false;
			for (u32 k = 0; k < keywords32.size(); k++)
			{
				const auto& kw = keywords32[k].keyword;
				if (c + kw.size() <= line.size())
				{
					// Boundary check
					bool bound = true;
					if (c > 0) { u32 prev = line[c - 1]; if ((prev >= 'a' && prev <= 'z') || (prev >= 'A' && prev <= 'Z') || (prev >= '0' && prev <= '9') || prev == '_') bound = false; }
					if (bound && c + kw.size() < line.size()) { u32 next = line[c + kw.size()]; if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z') || (next >= '0' && next <= '9') || next == '_') bound = false; }
					
					if (bound)
					{
						bool match = true;
						for (size_t i = 0; i < kw.size(); i++) if (line[c + i] != kw[i]) { match = false; break; }
						if (match)
						{
							appendSegment((i32)c, defaultColor);
							appendSegment((i32)c + (i32)kw.size(), keywords32[k].info->color);
							c += kw.size();
							keywordMatched = true;
							break;
						}
					}
				}
			}

			if (!keywordMatched)
			{
				i32 bestRule = -1;
				for (u32 r = 0; r < rules32.size(); r++)
				{
					const auto& startKw = rules32[r].begin;
					if (startKw.empty()) continue;
					bool match = true;
					if (c + startKw.size() > line.size()) match = false;
					else for (size_t k = 0; k < startKw.size(); k++) if (line[c + k] != startKw[k]) { match = false; break; }
					if (match) { bestRule = (i32)r; break; }
				}

				if (bestRule != -1)
				{
					appendSegment((i32)c, defaultColor);
					currentState = bestRule;
					c += rules32[bestRule].begin.size();
				}
				else c++;
			}
		}
	}
	appendSegment((i32)line.size(), (initialState != -1) ? rules32[initialState].info->color : defaultColor);

	auto end_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = end_time - start_time;
	if (elapsed.count() > 0.1) // Only print if it's significant to avoid spam
		printf("calculateSegments took %.3f ms\n", elapsed.count());
}

}
