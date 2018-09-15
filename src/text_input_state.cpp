#include "text_input_state.h"
#include "util.h"
#include "ui_font.h"

namespace hui
{
TextInputState::TextInputState()
{}

bool TextInputState::processEvent(const InputEvent& ev)
{
	textChanged = false;

	if (!widgetId)
	{
		return false;
	}

	if (ev.type == InputEvent::Type::MouseDown)
	{
		// we're out, no more text editing
		if (!rect.contains(ev.mouse.point))
		{
			widgetId = 0;
			return false;
		}

		mouseDown = true;
		caretPosition = getCharIndexAtX(ev.mouse.point.x);
		mouseDownSelectionBegin = caretPosition;
		mouseMoved = false;
		selectionBegin = selectionEnd = caretPosition;
		selectingWithMouse = false;
		computeScrollAmount();
		setCapture(0);
	}
	else if (ev.type == InputEvent::Type::MouseUp)
	{
		mouseDown = false;
		selectingWithMouse = false;
		computeScrollAmount();
		releaseCapture();

		if (!mouseMoved && firstMouseDown)
		{
			selectAll();
			firstMouseDown = false;
		}
	}
	else if (ev.window == getFocusedWindow())
	{
		auto chrPos = getCharIndexAtX(ev.mouse.point.x);

		if (mouseDown)
		{
			mouseDown = false;

			if (abs(chrPos - caretPosition))
				mouseMoved = true;

			selectingWithMouse = true;
		}

		if (selectingWithMouse)
		{
			selectionEnd = caretPosition = chrPos;
			selectionActive = true;
			computeScrollAmount();
		}
	}

	if (ev.type == InputEvent::Type::MouseDown
		&& ev.mouse.clickCount == 2)
	{
		if (ev.mouse.button == MouseButton::Left)
		{
			deselect();
			caretPosition = getCharIndexAtX(ev.mouse.point.x);
			selectionActive = true;

			if (!text.empty())
			{
				// select a word
				while (caretPosition >= 0 && text.size() > caretPosition)
				{
					if (text[caretPosition] == ' ')
					{
						selectionBegin = caretPosition;
						break;
					}

					caretPosition--;
				}

				if (caretPosition < 0)
				{
					caretPosition = 0;
				}

				if (caretPosition < text.size())
				{
					if (text[caretPosition] == ' ')
					{
						caretPosition++;
						selectionBegin++;
					}
				}

				while (caretPosition < text.size())
				{
					if (text[caretPosition] == ' ')
					{
						selectionEnd = caretPosition;
						break;
					}

					caretPosition++;
				}

				if (caretPosition == text.size())
				{
					selectionEnd = caretPosition;
				}
			}
		}
	}

	if (ev.type == InputEvent::Type::Text)
	{
		UnicodeString txt;

		utf8ToUtf32(ev.text.text, txt);
		formatValue(txt);
		insertTextAtCaret(txt);
		textChanged = true;
	}
	else if (ev.type == InputEvent::Type::Key)
	{
		if (ev.key.down)
		{
			if (ev.key.code == KeyCode::Enter)
			{
				formatValue(text);
				textChanged = true;
			}
		}

		processKeyEvent(ev);
	}

	return true;
}

void TextInputState::deleteSelection()
{
	UnicodeString str1, str2;
	int startSel = selectionBegin, endSel = selectionEnd, tmpSel;

	if (startSel > endSel)
	{
		tmpSel = startSel;
		startSel = endSel;
		endSel = tmpSel;
	}

	if (!text.empty())
	{
		text.erase(text.begin() + startSel, text.begin() + endSel);
	}

	startSel = startSel < text.size() ? startSel : text.size();
	caretPosition = startSel;
	selectionBegin = selectionEnd = caretPosition;
	selectionActive = false;
	textChanged = true;
}

UnicodeString TextInputState::getSelection()
{
	if (selectionActive)
	{
		i32 startSel = selectionBegin, endSel = selectionEnd, tmpSel;

		if (startSel > endSel)
		{
			tmpSel = startSel;
			startSel = endSel;
			endSel = tmpSel;
		}

		if (!text.empty() && abs(endSel - startSel))
		{
			UnicodeString str(text.begin() + startSel, text.begin() + endSel);
			return str;
		}
	}

	return UnicodeString();
}

void TextInputState::clearText()
{
	text.clear();
	selectionBegin = selectionEnd = caretPosition = 0;
	selectionActive = false;
	scrollOffset = 0;
	textChanged = true;
}

void TextInputState::processKeyEvent(const InputEvent& ev)
{
	if (ev.type != InputEvent::Type::Key)
		return;

	if (ev.key.code == KeyCode::None)
		return;

	// only key downs
	if (!ev.key.down)
		return;

	if (ev.key.code == KeyCode::ArrowLeft)
	{
		caretPosition--;

		if (caretPosition < 0)
			caretPosition = 0;

		// do we select ?
		if (!!(ev.key.modifiers & KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionBegin = caretPosition + 1;
				selectionEnd = caretPosition;
			}
			else
			{
				selectionEnd = caretPosition;
			}
		}
		else
		{
			selectionActive = false;
		}
	}
	else if (ev.key.code == KeyCode::ArrowRight)
	{
		caretPosition++;

		if (caretPosition > text.size())
			caretPosition = text.size();

		// do we select ?
		if (!!(ev.key.modifiers & KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionBegin = caretPosition - 1;
				selectionEnd = caretPosition;
			}
			else
			{
				selectionEnd = caretPosition;
			}
		}
		else
		{
			selectionActive = false;
		}
	}
	else if (ev.key.code == KeyCode::Delete)
	{
		if (selectionActive)
		{
			deleteSelection();
		}
		else
		{
			if (!text.empty() && text.size() > caretPosition)
			{
				text.erase(text.begin() + caretPosition);
				textChanged = true;
			}
		}
	}
	else if (ev.key.code == KeyCode::Backspace)
	{
		if (selectionActive)
		{
			deleteSelection();
		}
		else
		{
			if (!text.empty())
			{
				if (caretPosition > 0)
				{
					int cart = caretPosition;

					cart--;

					if (caretPosition >= text.size())
						cart = text.size() - 1;

					text.erase(text.begin() + cart);
					--caretPosition;
					textChanged = true;
				}

				if (caretPosition < 0)
					caretPosition = 0;
			}

			if (caretPosition < 0)
			{
				caretPosition = 0;
			}
		}
	}
	else if (ev.key.code == KeyCode::End || ev.key.code == KeyCode::ArrowDown)
	{
		// do we select ?
		if (!!(ev.key.modifiers & KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionBegin = caretPosition;
				selectionEnd = text.size();
			}
			else
			{
				selectionEnd = text.size();
			}
		}
		else
		{
			selectionActive = false;
		}

		caretPosition = text.size();
	}
	else if (ev.key.code == KeyCode::Home || ev.key.code == KeyCode::ArrowUp)
	{
		// do we select ?
		if (!!(ev.key.modifiers & KeyModifiers::Shift))
		{
			if (!selectionActive)
			{
				selectionActive = true;
				selectionBegin = caretPosition;
				selectionEnd = 0;
			}
			else
			{
				selectionEnd = 0;
			}
		}
		else
		{
			selectionActive = false;
		}

		caretPosition = 0;
		scrollOffset = 0;
	}
	else if (ev.key.code == KeyCode::A && !!(ev.key.modifiers & KeyModifiers::Control))
	{
		selectionActive = true;
		selectionBegin = 0;
		selectionEnd = text.size();
	}
	else if (ev.key.code == KeyCode::C && !!(ev.key.modifiers & KeyModifiers::Control))
	{
		UnicodeString str = getSelection();

		if (!str.empty())
		{
			char* tmpStr = 0;

			utf32ToUtf8(str, &tmpStr);
			copyToClipboard(tmpStr);
			delete[] tmpStr;
		}
	}
	else if (ev.key.code == KeyCode::V && !!(ev.key.modifiers & KeyModifiers::Control))
	{
		if (selectionActive)
		{
			deleteSelection();
		}

		static const u32 maxTextSize = 2048;
		char tmpStr[maxTextSize];
		UnicodeString utf32Str;

		pasteFromClipboard(tmpStr, maxTextSize);

		if (utf8ToUtf32(tmpStr, utf32Str))
		{
			delete[] tmpStr;

			if (utf32Str.size() + text.size() < maxTextLength)
			{
				int offs = caretPosition;

				if (caretPosition > text.size())
				{
					offs = text.size() - 1;
				}

				formatValue(utf32Str);
				text.insert(text.begin() + offs, utf32Str.begin(), utf32Str.end());
				caretPosition += utf32Str.size();
				textChanged = true;
			}
		}
	}
	else if (ev.key.code == KeyCode::X && !!(ev.key.modifiers & KeyModifiers::Control))
	{
		UnicodeString str = getSelection();

		if (selectionActive && !str.empty())
		{
			deleteSelection();

			char* str8 = 0;

			if (utf32ToUtf8(str, &str8))
			{
				copyToClipboard(str8);
				delete[] str8;
				textChanged = true;
			}
		}
	}

	if (caretPosition > text.size())
	{
		caretPosition = text.size();
	}

	computeScrollAmount();
}

void TextInputState::insertTextAtCaret(const UnicodeString& newText)
{
	if (selectionActive)
		deleteSelection();

	if (newText.empty())
	{
		return;
	}

	int offs = caretPosition;

	if (caretPosition > text.size() && !text.empty())
		offs = text.size() - 1;

	if (text.size() + newText.size() < maxTextLength)
	{
		text.insert(text.begin() + offs, newText.begin(), newText.end());
		caretPosition += newText.size();
		textChanged = true;
	}

	selectionActive = false;
}

i32 TextInputState::getCharIndexAtX(f32 xPos)
{
	FontTextSize rcCurrentRange, rcLastChar;
	UnicodeString subStr, lastChar;
	i32 pos = 0;

	xPos += scrollOffset;

	UiFont* font = (UiFont*)themeElement->normalState().font;

	if (!font)
	{
		return 0;
	}

	//TODO: optimize, slow on long text
	for (size_t i = 0; i < text.size(); ++i)
	{
		subStr = UnicodeString(text.begin(), text.begin() + i);

		if (subStr.size() >= 1)
		{
			lastChar = UnicodeString(subStr.begin() + (subStr.size() - 1), subStr.end());

			rcLastChar = font->computeTextSize(lastChar.data(), 1);
		}
		else
		{
			rcLastChar.width = 0;
		}

		rcCurrentRange = font->computeTextSize(subStr.data(), subStr.size());
		bool passed = false;

		// if X is to the left of the middle of last char
		if (((clipRect.x + rcCurrentRange.width) - rcLastChar.width / 2.0f) >= xPos)
		{
			pos = i - 1;
			passed = true;
		}
		// if X is on the right of the middle of last char
		else if (clipRect.x + rcCurrentRange.width >= xPos)
		{
			pos = i;
			passed = true;
		}

		if (passed)
		{
			if (pos < 0)
				pos = 0;

			if (pos > text.size())
				pos = text.size();

			return pos;
		}
	}

	return text.size();
}

void TextInputState::computeScrollAmount()
{
	UnicodeString tmpStr = UnicodeString(text.begin(), text.begin() + caretPosition);
	FontTextSize textSizeToCaret = themeElement->normalState().font->computeTextSize(tmpStr.data(), tmpStr.size());
	f32 absCursorPosX = clipRect.x + textSizeToCaret.width - scrollOffset;
	f32 stepAmount = 30;

	if (absCursorPosX < clipRect.x)
	{
		scrollOffset -= (clipRect.x - absCursorPosX) + stepAmount;
	}
	else if (absCursorPosX > clipRect.right())
	{
		scrollOffset += absCursorPosX - clipRect.right() + stepAmount;
	}

	if (scrollOffset < 0)
	{
		scrollOffset = 0;
	}
}

void TextInputState::formatValue(UnicodeString& value)
{
	switch (valueType)
	{
	case TextInputValueMode::NumericOnly:
	{
		std::vector<u32>::iterator iter = value.begin();

		while (iter != value.end())
		{
			if (!isdigit(*iter)
				&& *iter != '.' && *iter != ',' 
				&& *iter != '-' && *iter != 'e'
				&& *iter != 'E' && *iter != '+')
			{
				iter = value.erase(iter);
				continue;
			}

			iter++;
		}

		break;
	}
	default:
		break;
	}
}

}
