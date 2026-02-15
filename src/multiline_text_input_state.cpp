#include "multiline_text_input_state.h"
#include "util.h"
#include "font.h"
#include "context.h"
#include "theme.h"

namespace hui
{
MultilineTextInputState::MultilineTextInputState()
{
	lines.push_back(Utf32String()); // Start with one empty line
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

	// Normalize selection
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
		// Single line deletion
		lines[startLine].erase(lines[startLine].begin() + startCol, lines[startLine].begin() + endCol);
	}
	else
	{
		// Multi-line deletion
		// Keep text before selection on start line and text after selection on end line
		Utf32String remainingText = Utf32String(lines[startLine].begin(), lines[startLine].begin() + startCol);
		Utf32String afterSelection(lines[endLine].begin() + endCol, lines[endLine].end());
		remainingText.insert(remainingText.end(), afterSelection.begin(), afterSelection.end());

		// Remove all lines in between
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

	// Multi-line selection
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

	// Handle newlines in pasted text
	for (u32 ch : newText)
	{
		if (ch == '\n' || ch == '\r')
		{
			// Split line at caret
			Utf32String remaining(lines[currentLine].begin() + caretColumn, lines[currentLine].end());
			lines[currentLine].erase(lines[currentLine].begin() + caretColumn, lines[currentLine].end());
			currentLine++;
			lines.insert(lines.begin() + currentLine, remaining);
			caretColumn = 0;
		}
		else
		{
			lines[currentLine].insert(lines[currentLine].begin() + caretColumn, ch);
			caretColumn++;
		}
	}

	textChanged = true;
}

Point MultilineTextInputState::getCaretScreenPosition()
{
	auto& elemState = themeElement->normalState();
	Font* font = elemState.font;

	if (!font || currentLine >= lines.size())
		return Point(clipRect.x, clipRect.y);

	Utf32String textToCaretlinear(lines[currentLine].begin(),
		lines[currentLine].begin() + std::min((i32)lines[currentLine].size(), caretColumn));

	FontTextSize textSize = font->computeTextSize(textToCaretlinear.data(), (u32)textToCaretlinear.size());
	f32 lineHeight = font->getMetrics().height;

	return Point(
		clipRect.x + textSize.width - scrollOffsetX,
		clipRect.y + currentLine * lineHeight - scrollOffsetY
	);
}

void MultilineTextInputState::computeScrollAmount()
{
	Point caretPos = getCaretScreenPosition();
	auto& elemState = themeElement->normalState();
	Font* font = elemState.font;
	f32 lineHeight = font ? font->getMetrics().height : 20.0f;

	// Calculate total content size
	f32 totalHeight = lines.size() * lineHeight;
	f32 maxLineWidth = 0;

	for (const auto& line : lines)
	{
		FontTextSize size = font->computeTextSize(line.data(), (u32)line.size());
		if (size.width > maxLineWidth)
			maxLineWidth = size.width;
	}

	// Clamp Vertical scrolling
	f32 maxScrollY = std::max(0.0f, totalHeight - clipRect.height + lineHeight); // Allow scrolling one line past end
	
	if (scrollOffsetY < 0) scrollOffsetY = 0;
	if (scrollOffsetY > maxScrollY) scrollOffsetY = maxScrollY;

	// Clamp Horizontal scrolling
	f32 maxScrollX = std::max(0.0f, maxLineWidth - clipRect.width + 20.0f); // Add some padding

	if (scrollOffsetX < 0) scrollOffsetX = 0;
	if (scrollOffsetX > maxScrollX) scrollOffsetX = maxScrollX;
}

void MultilineTextInputState::ensureCaretVisible()
{
	Point caretPos = getCaretScreenPosition();
	auto& elemState = themeElement->normalState();
	Font* font = elemState.font;
	f32 lineHeight = font ? font->getMetrics().height : 20.0f;

	// Auto-scroll to caret if needed (only if caret moved)
	if (caretPos.x < clipRect.x)
		scrollOffsetX -= (clipRect.x - caretPos.x) + 10;
	else if (caretPos.x > clipRect.right())
		scrollOffsetX += (caretPos.x - clipRect.right()) + 10;

	// Calculate effective line height including spacing if any
	// (currently text input uses tight line spacing, but let's be safe)
	
	if (caretPos.y < clipRect.y)
	{
		scrollOffsetY -= (clipRect.y - caretPos.y);
	}
	else if (caretPos.y + lineHeight > clipRect.bottom())
	{
		// Scroll enough to show the full line plus a small margin to avoid feeling cramped
		// Using 2.0f as a safe margin
		scrollOffsetY += (caretPos.y + lineHeight - clipRect.bottom()) + 2.0f;
	}
	
	computeScrollAmount();
}

void MultilineTextInputState::formatValue()
{
	// No formatting for multiline (could add later if needed)
}

i32 MultilineTextInputState::getCharIndexAtPoint(const Point& pt)
{
	Font* font = themeElement ? themeElement->normalState().font : nullptr;
	if (!font)
		return 0;

	f32 lineHeight = font->getMetrics().height;
	f32 adjustedY = pt.y + scrollOffsetY - clipRect.y;
	i32 line = (i32)(adjustedY / lineHeight);

	if (line < 0) line = 0;
	if (line >= lines.size()) line = lines.size() - 1;

	currentLine = line;

	// Find column
	f32 adjustedX = pt.x + scrollOffsetX;
	const auto& lineText = lines[line];

	for (size_t i = 0; i <= lineText.size(); i++)
	{
		Utf32String substr(lineText.begin(), lineText.begin() + i);
		FontTextSize size = font->computeTextSize(substr.data(), (u32)substr.size());

		if (clipRect.x + size.width >= adjustedX)
		{
			caretColumn = (i > 0) ? i - 1 : 0;
			return caretColumn;
		}
	}

	caretColumn = lineText.size();
	return caretColumn;
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
		ensureCaretVisible();
		releaseWindowCapture();

		if (!mouseMoved && firstMouseDown && !selectingWithMouse)
		{
			if (has(flags, TextInputFlags::AutoSelectAll) && selectAllOnFocus)
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
		Utf32String txt;
		ctx->settings.services.utf8To32(ev.text.text, txt);
		insertTextAtCaret(txt);
		textChanged = true;
	}
	else if (ev.type == InputEvent::Type::Key)
	{
		processKeyEvent(ev);
	}
	else if (ev.type == InputEvent::Type::MouseWheel)
	{
		if (rect.contains(ctx->mousePosition))
		{
			f32 scrollAmountY = ev.mouse.wheel.y * 30.0f * ctx->scale; // TODO: make speed configurable
			f32 scrollAmountX = ev.mouse.wheel.x * 30.0f * ctx->scale;

			if (has(ev.mouse.modifiers, KeyModifiers::Shift))
			{
				std::swap(scrollAmountX, scrollAmountY);
			}

			scrollOffsetY -= scrollAmountY;
			scrollOffsetX -= scrollAmountX;

			computeScrollAmount();
		}
	}

	return true;
}

void MultilineTextInputState::processKeyEvent(const InputEvent& ev)
{
	if (ev.type != InputEvent::Type::Key || !ev.key.down)
		return;

	Font* font = themeElement ? themeElement->normalState().font : nullptr;
	bool hasSelection = selectionActive;

	if (ev.key.code == KeyCode::ArrowLeft)
	{
		if (caretColumn > 0)
			caretColumn--;
		else if (currentLine > 0)
		{
			currentLine--;
			caretColumn = lines[currentLine].size();
		}

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = currentLine;
				selectionStartColumn = caretColumn + 1;
			}
			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
			deselect();
	}
	else if (ev.key.code == KeyCode::ArrowRight)
	{
		if (caretColumn < lines[currentLine].size())
			caretColumn++;
		else if (currentLine < lines.size () - 1)
		{
			currentLine++;
			caretColumn = 0;
		}

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = currentLine;
				selectionStartColumn = caretColumn - 1;
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

		if (currentLine > 0)
			currentLine--;

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = prevLine;
				selectionStartColumn = prevColumn;
			}

			// If we're at the first line and couldn't move up, move caret to start of line
			if (currentLine == 0 && prevLine == 0)
			{
				caretColumn = 0;
			}
			else
			{
				// Clamp caret column to new line's size if needed
				if (caretColumn > lines[currentLine].size())
					caretColumn = lines[currentLine].size();
			}

			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
		{
			if (caretColumn > lines[currentLine].size())
				caretColumn = lines[currentLine].size();

			deselect();
		}
	}
	else if (ev.key.code == KeyCode::ArrowDown)
	{
		i32 prevLine = currentLine;
		i32 prevColumn = caretColumn;

		if (currentLine < lines.size() - 1)
			currentLine++;

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = prevLine;
				selectionStartColumn = prevColumn;
			}

			// If we're at the last line and couldn't move down, move caret to end of line
			if (currentLine == lines.size() - 1 && prevLine == currentLine)
			{
				caretColumn = lines[currentLine].size();
			}
			else
			{
				// Clamp caret column to new line's size if needed
				if (caretColumn > lines[currentLine].size())
					caretColumn = lines[currentLine].size();
			}

			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
		{
			if (caretColumn > lines[currentLine].size())
				caretColumn = lines[currentLine].size();

			deselect();
		}
	}
	else if (ev.key.code == KeyCode::Enter)
	{
		// Create new line
		Utf32String remaining(lines[currentLine].begin() + caretColumn, lines[currentLine].end());
		lines[currentLine].erase(lines[currentLine].begin() + caretColumn, lines[currentLine].end());
		currentLine++;
		lines.insert(lines.begin() + currentLine, remaining);
		caretColumn = 0;
		deselect();
		textChanged = true;
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
			// Merge with previous line
			caretColumn = lines[currentLine - 1].size();
			lines[currentLine - 1].insert(lines[currentLine - 1].end(), lines[currentLine].begin(), lines[currentLine].end());
			lines.erase(lines.begin() + currentLine);
			currentLine--;
			textChanged = true;
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
			// Merge with next line
			lines[currentLine].insert(lines[currentLine].end(), lines[currentLine + 1].begin(), lines[currentLine + 1].end());
			lines.erase(lines.begin() + currentLine + 1);
			textChanged = true;
		}
	}
	else if (ev.key.code == KeyCode::Home)
	{
		i32 prevColumn = caretColumn;
		caretColumn = 0;

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = currentLine;
				selectionStartColumn = prevColumn;
			}
			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
			deselect();
	}
	else if (ev.key.code == KeyCode::End)
	{
		i32 prevColumn = caretColumn;
		caretColumn = lines[currentLine].size();

		if (has(ev.key.modifiers, KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionStartLine = currentLine;
				selectionStartColumn = prevColumn;
			}
			selectionEndLine = currentLine;
			selectionEndColumn = caretColumn;
		}
		else
			deselect();
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
	else if (ev.key.code == KeyCode::Tab)
	{
		// Insert tab character (or spaces)
		Utf32String tabStr;
		tabStr.push_back('\t');
		insertTextAtCaret(tabStr);
		textChanged = true;
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
