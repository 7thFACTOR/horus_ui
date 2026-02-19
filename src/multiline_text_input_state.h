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
	void formatValue();
	i32 getCharIndexAtPoint(const Point& pt);
	void ensureCaretVisible();

	struct ThemeElement* themeElement = nullptr;
	Rect rect;
	Rect clipRect;
	Rect scrollbarRectV;
	Rect scrollbarRectH;
	std::vector<Utf32String> lines; // Each line is a separate string
	MultilineTextInputFlags flags = MultilineTextInputFlags::None;

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
	// Handled by ScrollView widget now
	f32 scrollOffsetX = 0;
	f32 scrollOffsetY = 0;
	void computeScrollAmount();
	WidgetId scrollId = 0;
	i32 firstVisibleLine = 0;

	// Mouse state
	bool firstMouseDown = true;
	bool mouseDown = false;
	bool mouseMoved = false;
	bool caretPreferLineEnd = false;

	// Misc
	bool selectAllOnFocus = true;
	bool textChanged = false;
	u32 maxTextLength = 0;
	f32 caretBlinkTimer = 0;
	u32 visibleLineCount = 10; // How many lines to display

	// Duplicate event protection
	u32 lastKeyProcessFrame = 0;

	struct VisualLine
	{
		i32 logicalLineIndex = 0;
		i32 startColumn = 0;
		i32 length = 0;
		f32 width = 0;
	};

	std::vector<VisualLine> visualLines;
	void computeVisualLines(class Font* font, f32 availableWidth);
	f32 lastLayoutWidth = 0.0f;
};

}
