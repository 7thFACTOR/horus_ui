#pragma once
#include "types.h"

namespace hui
{
struct TextInputState
{
	u32 widgetId = 0;
	bool editNow = false;

	struct EditKeyShiftInfo
	{
		EditKeyShiftInfo(char chr = 0, char shiftedChar = 0)
		{
			charCode = chr;
			shiftedCharCode = shiftedChar;
		}

		u32 charCode, shiftedCharCode;
	};

	TextInputState();

	void selectAll() { selectionBegin = 0; selectionEnd = text.size(); selectionActive = true; caretPosition = text.size(); }
	void deselect() { selectionBegin = 0; selectionEnd = 0; selectionActive = false; }
	void deleteSelection();
	UnicodeString getSelection();
	void clearText();
	void insertTextAtCaret(const UnicodeString& newText);
	bool processEvent(const InputEvent& ev);
	void processKeyEvent(const InputEvent& ev);
	i32 getCharIndexAtX(f32 aX);
	void computeScrollAmount();
	void formatValue(UnicodeString& value);

	struct UiThemeElement* themeElement;
	Rect rect;
	Rect clipRect;
	UnicodeString text;
	UnicodeString defaultText;
	TextInputValueMode valueType = TextInputValueMode::Any;
	u32 decimalPlaces = 2;
	i32 selectionBegin = 0;
	i32 mouseDownSelectionBegin = 0;
	i32 selectionEnd = 0;
	f32 scrollOffset = 0;
	i32 caretPosition = 0;
	bool firstMouseDown = true;
	bool mouseDown = false;
	bool mouseMoved = false;
	bool selectionActive = false;
	bool selectingWithMouse = false;
	bool selectAllOnFocus = true;
	bool textChanged = false;
	u32 maxTextLength = 0;
};

}