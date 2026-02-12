#include "text_input_state.h"
#include "util.h"
#include "font.h"
#include "context.h"
#include "theme.h"

namespace hui
{
TextInputState::TextInputState()
{}

bool TextInputState::processEvent(const InputEvent& ev)
{
	textChanged = false;
	clearFilterHovered = false;

	if (!id)
	{
		return false;
	}

	const Point& mousePos = ctx->mousePosition;

	// compute clear-filter hit rect (match how the widget draws the image)
	Rect clearFilterRc = rect;
	auto& bodyTextFilterClearImageElem = ctx->theme->getElement(WidgetElementId::TextInputFilterClearImage);
	auto& bodyTextElem = ctx->theme->getElement(WidgetElementId::TextInputBody);

	// image size in screen pixels
	auto imgW = bodyTextFilterClearImageElem.normalState().image->width * ctx->scale;
	auto imgH = bodyTextFilterClearImageElem.normalState().image->height * ctx->scale;

	// position image at same X/Y used by the widget rendering
	clearFilterRc.x = rect.right() - (bodyTextElem.normalState().border + getWidgetPadding().x) * ctx->scale - imgW;
	clearFilterRc.width = imgW;
	clearFilterRc.y = rect.y + (rect.height - imgH) / 2.0f;
	clearFilterRc.height = imgH;

	clearFilterHovered = clearFilterRc.contains(mousePos);

	if (ev.type == InputEvent::Type::MouseDown)
	{
		// we're out, no more text editing
		if (!rect.contains(ev.mouse.point))
		{
			id = 0;
			return false;
		}

		mouseDown = true;
		caretPosition = getCharIndexAtX(ev.mouse.point.x);
		mouseDownSelectionBegin = caretPosition;
		mouseMoved = false;
		selectionBegin = selectionEnd = caretPosition;
		selectingWithMouse = false;
		computeScrollAmount();
		setWindowCapture();
	}
	else if (ev.type == InputEvent::Type::MouseUp)
	{
		computeScrollAmount();
		releaseWindowCapture();

		if (!mouseMoved && firstMouseDown && !selectingWithMouse)
		{
			if (has(flags, TextInputFlags::AutoSelectAll) && selectAllOnFocus)
				selectAll();
			firstMouseDown = false;
		}

		if (mouseDown && clearFilterHovered)
		{
			clearText();
			textChanged = true;
		}

		mouseDown = false;
		selectingWithMouse = false;
	}
	else if (ev.window == ctx->settings.services.getFocusedWindow())
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

	if (ev.type == InputEvent::Type::MouseDown
		&& ev.mouse.clickCount == 3)
	{
		if (ev.mouse.button == MouseButton::Left)
		{
			selectAll();
		}
	}

	if (ev.type == InputEvent::Type::Text)
	{
		Utf32String txt;

		ctx->settings.services.utf8To32(ev.text.text, txt);
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
	Utf32String str1, str2;
	i32 startSel = selectionBegin, endSel = selectionEnd, tmpSel;

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

Utf32String TextInputState::getSelection()
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
			Utf32String str(text.begin() + startSel, text.begin() + endSel);
			return str;
		}
	}

	return Utf32String();
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
					i32 newCaretPos = caretPosition;

					newCaretPos--;

					if (caretPosition >= text.size())
						newCaretPos = text.size() - 1;

					text.erase(text.begin() + newCaretPos);
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
		if (has(ev.key.modifiers, KeyModifiers::Shift))
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
		if (has(ev.key.modifiers, KeyModifiers::Shift))
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
	else if (!password && ev.key.code == KeyCode::C && !!(ev.key.modifiers & KeyModifiers::Control))
	{
		Utf32String str = getSelection();

		if (!str.empty())
		{
			char* tmpStr = 0;

			ctx->settings.services.utf32To8(str, &tmpStr);
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
		Utf32String utf32Str;

		pasteFromClipboard(tmpStr, maxTextSize);

		if (ctx->settings.services.utf8To32(tmpStr, utf32Str))
		{
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
		Utf32String str = getSelection();

		if (selectionActive && !str.empty())
		{
			deleteSelection();

			char* str8 = 0;

			if (ctx->settings.services.utf32To8(str, &str8))
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

void TextInputState::insertTextAtCaret(const Utf32String& newText)
{
	if (selectionActive)
		deleteSelection();

	if (newText.empty())
	{
		return;
	}

	i32 offs = caretPosition;

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
	Utf32String subStr, lastChar;
	i32 pos = 0;

	xPos += scrollOffset;

	Font* font = (Font*)themeElement->normalState().font;

	if (!font)
	{
		return 0;
	}

	//TODO: optimize, slow on long text
	for (size_t i = 0; i < text.size(); ++i)
	{
		subStr = Utf32String(text.begin(), text.begin() + i);

		if (subStr.size() >= 1)
		{
			lastChar = Utf32String(subStr.begin() + (subStr.size() - 1), subStr.end());

			if (!password)
			{
				rcLastChar = font->computeTextSize(lastChar.data(), 1);
			}
			else
			{
				rcLastChar = font->computeTextSize(passwordCharUnicode.data(), (u32)passwordCharUnicode.size());
			}
		}
		else
		{
			rcLastChar.width = 0;
		}

		if (!password)
		{
			rcCurrentRange = font->computeTextSize(subStr.data(), (u32)subStr.size());
		}
		else
		{
			rcCurrentRange = font->computeTextSize(passwordCharUnicode.data(), passwordCharUnicode.size());
			rcCurrentRange.width *= subStr.size();
		}

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
	Utf32String tmpStr = Utf32String(text.begin(), text.begin() + caretPosition);
	FontTextSize textSizeToCaret;

	if (!password)
	{
		textSizeToCaret = themeElement->normalState().font->computeTextSize(tmpStr.data(), (u32)tmpStr.size());
	}
	else
	{
		textSizeToCaret = themeElement->normalState().font->computeTextSize(passwordCharUnicode.data(), passwordCharUnicode.size());
		textSizeToCaret.width *= tmpStr.size();
	}

	f32 absCursorPosX = clipRect.x + textSizeToCaret.width - scrollOffset;

	if (absCursorPosX < clipRect.x)
	{
		scrollOffset -= (clipRect.x - absCursorPosX) + ctx->settings.textScrollStepAmount;
	}
	else if (absCursorPosX > clipRect.right())
	{
		scrollOffset += absCursorPosX - clipRect.right() + ctx->settings.textScrollStepAmount;
	}

	if (scrollOffset < 0)
	{
		scrollOffset = 0;
	}
}

void TextInputState::formatValue(Utf32String& value)
{
	if (has(flags, TextInputFlags::NumericOnly))
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
	}
	else if (has(flags, TextInputFlags::HexOnly))
	{
		std::vector<u32>::iterator iter = value.begin();

		while (iter != value.end())
		{
			if (!isxdigit(*iter))
			{
				iter = value.erase(iter);
				continue;
			}

			if (islower(*iter))
			{
				*iter = toupper(*iter);
				continue;
			}

			iter++;
		}
	}
}

}
