#pragma once
#include "types.h"
#include "undo.h"
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

	// undo/redo support (per-widget)
	UndoStack undoStack;

	// Coalescing continuous typing on same logical line into a single undo step.
	// When true, single-character inserts on the same line at the expected insertion
	// column will NOT push a new snapshot (they're grouped into the previously pushed snapshot).
	bool undoTypingActive = false;
	i32 undoTypingLine = -1;
	i32 undoTypingNextColumn = -1;

	// Duplicate event protection
	u32 lastKeyProcessFrame = 0;

	// Helpers for undo
	void pushUndoSnapshot(); // push current state into undo stack
	void applyUndoSnapshot(const UndoStack::State& s); // restore a snapshot and mark dirty

	struct VisualSegment
	{
		i32 length = 0;
		Color color;
	};

	struct VisualLine
	{
		i32 logicalLineIndex = 0;
		i32 startColumn = 0;
		i32 length = 0;
		f32 width = 0;
		std::vector<VisualSegment> segments;
	};

	std::vector<const VisualLine*> visualLines;
	std::vector<std::vector<VisualLine>> lineVisuals; // Per-logical-line cache
	void computeVisualLines(class Font* font, f32 availableWidth, 
		const struct RangeHighlight* rules, u32 ruleCount, 
		const struct KeywordInfo* keywords, u32 keywordCount);
	f32 lastLayoutWidth = 0.0f;
	f32 maxLineWidth = 0.0f;
	size_t caretVisualLineIndex = (size_t)-1;

	// Syntax highlighting state
	std::vector<i32> lineStates; // Index of active range highlight at start of line, -1 if none
	u64 lastRulesHash = 0;
	const struct RangeHighlight* lastRulesPtr = nullptr;
	const struct KeywordInfo* lastKeywordsPtr = nullptr;
	u32 lastRuleCount = 0;
	u32 lastKeywordCount = 0;
	std::vector<VisualSegment> tempSegments;
	void updateSyntaxHighlighting(const struct RangeHighlight* rules, u32 count, const struct KeywordInfo* keywords, u32 keywordCount);

	struct Rule32 {
		Utf32String begin;
		Utf32String end;
		Utf32String escape;
		const struct RangeHighlight* info;
	};
	std::vector<Rule32> rules32;

	struct Keyword32 {
		Utf32String keyword;
		const KeywordInfo* info;
	};
	std::vector<Keyword32> keywords32;

	void calculateSegments(const Utf32String& line, i32 initialState, std::vector<VisualSegment>& outSegments,
		const struct RangeHighlight* rules, u32 ruleCount, 
		const struct KeywordInfo* keywords, u32 keywordCount);
	f32 calculateTextSegmentWidth(class Font* font, const Utf32String& line, i32 startCol, i32 length, i32 logicalLineIndex);
	std::vector<char> utf8LineBuffer;

	// Incremental updates
	i32 firstDirtyLine = -1;
	bool forceLayoutUpdate = false;
	bool lastHasScrollbarV = false;
	size_t totalTextLength = 0;
	void markLineDirty(i32 logicalLineIndex);
};

}
