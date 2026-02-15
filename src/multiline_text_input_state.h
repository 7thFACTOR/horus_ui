#pragma once
#include "types.h"
#include <vector>

namespace hui
{
struct MultilineTextInputState
{
	WidgetId id = 0;
	bool editNow = false;

	MultilineTextInputState();

	void selectAll();
	void deselect();
	void deleteSelection();
	Utf32String getSelection();
	void clearText();
	void insertTextAtCaret(const Utf32String& newText);
	bool processEvent(const InputEvent& ev);
	void processKeyEvent(const InputEvent& ev);
	Point getCaretScreenPosition();
	void computeScrollAmount();
	void formatValue();
	i32 getCharIndexAtPoint(const Point& pt);
	void ensureCaretVisible();

	struct ThemeElement* themeElement = nullptr;
	Rect rect;
	Rect clipRect;
	std::vector<Utf32String> lines; // Each line is a separate string
	TextInputFlags flags = TextInputFlags::None;

	// Caret position
	i32 currentLine = 0;
	i32 caretColumn = 0; // Column within current line

	// Selection
	i32 selectionStartLine = 0;
	i32 selectionStartColumn = 0;
	i32 selectionEndLine = 0;
	i32 selectionEndColumn = 0;
	bool selectionActive = false;
	bool selectingWithMouse = false;
	i32 mouseDownSelectionStartLine = 0;
	i32 mouseDownSelectionStartColumn = 0;

	// Scrolling
	f32 scrollOffsetX = 0;
	f32 scrollOffsetY = 0; // Vertical scroll in pixels
	i32 firstVisibleLine = 0;

	// Mouse state
	bool firstMouseDown = true;
	bool mouseDown = false;
	bool mouseMoved = false;

	// Misc
	bool selectAllOnFocus = true;
	bool textChanged = false;
	u32 maxTextLength = 0;
	f32 caretBlinkTimer = 0;
	u32 visibleLineCount = 10; // How many lines to display
};

}
