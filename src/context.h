#pragma once
#include <chrono>
#include "types.h"
#include "text_input_state.h"
#include "multiline_text_input_state.h"
#include "renderer.h"
#include "atlas.h"

namespace hui
{
struct Context
{
	static const size_t maxLayerCount = 256;
	static const size_t maxNestingIndex = 256;
	static const size_t maxPopupIndex = 256;
	static const size_t maxMenuDepth = 256;
	static const size_t maxBoxDepth = 256;
	static const size_t maxSameLineInfoIndex = 256;

	Settings settings;
	Renderer renderer;

	// Global general state
	f32 totalTime = 0;
	u32 frameCount = 0;
	f32 pruneUnusedTextTime = 0; //TODO: maybe make it frames
	std::chrono::high_resolution_clock::time_point frameStartTime;
	f32 lastFrameTimeMs = 0.0f;
	f32 peakFrameTimeMs = 0.0f;
	f32 avgFrameTimeMs = 0.0f;
	f32 frameTimes[60] = {}; // Rolling buffer for average calculation
	u32 frameTimeIndex = 0;
	bool mustRedraw = false;
	bool focusChanged = false;
	bool skipRenderAndInput = false;
	bool hoveringThisWindow = false;
	Window* currentWindow = nullptr;
	WindowFlags nextWindowFlags = WindowFlags::None;
	HNativeWindow lastHoveredNativeWindow = nullptr;
	f32 scale = 1.0f;
	Point mousePosition;

	// Widgets
	WidgetId id = 42;
	WidgetState widget;
	TextInputState textInput;
	std::unordered_map<WidgetId, MultilineTextInputState> textMultilineInput;
	WidgetId activeMultilineInputId = 0;
	std::vector<TextLineState> textLines;
	std::vector<WidgetId> idStack;
	std::unordered_map<WidgetId, WidgetBoolState> widgetBools;

	// Same line
	SameLineState sameLine;
	SameLineGroupState sameLineGroup;

	TooltipState tooltip;
	std::string widgetLabel;
	bool verticalToolbar = false;
	std::vector<bool> verticalToolbarStack;

	// Layers
	size_t layerIndex = 0;
	size_t maxLayerIndex = 0;

	// Popups
	std::vector<PopupState> popupStack;
	bool popupUseGlobalScale = true;
	size_t popupIndex = 0;

	// Virtual list
	std::vector<VirtualListContentState> virtualListStack;

	// List
	std::unordered_map<u32, i32> listAnchors;

	// Menus
	std::vector<MenuWidgetState> menuStack;
	size_t menuDepth = 0;
	WidgetId activeMenuBarItemWidgetId = 0;
	bool contextMenuActive = false;
	bool contextMenuClicked = false;
	WidgetId contextMenuWidgetId = 0;
	std::string contextMenuNameId;
	bool menuItemChosen = false;
	bool pressedOnMenuItem = false;
	bool isSubMenu = false;
	bool clickedOnASubMenuItem = false;
	bool switchedToAnotherMainMenu = false;
	Point activeMenuBarItemWidgetPos;
	f32 activeMenuBarItemWidgetWidth = 0;
	bool rightSideMenu = true;
	size_t hoveredSimpleMenuItemMenuDepth = ~0;
	WidgetId activeMenuBarId = 0;
	WidgetId currentMenuBarId = 0;
	f32 menuItemTextWidth = 0;
	f32 menuItemTextSideSpacing = 10;
	f32 menuImageSpace = 18;
	f32 menuFillerWidth = 30;

	std::unordered_map<WidgetId, struct DrawCmdLayerSplitter> boxDrawCmdSplitter;
	std::unordered_map<WidgetId, BoxState> boxState;

	// Scrolling
	f32 maxContentWidth = 0;
	std::vector<f32> maxContentWidthStack;
	f32 scrollViewSpeed = 0.2f;
	f32 scrollViewScrollPageSize = 0.4f;
	WidgetId dragScrollViewHandleWidgetId = 0;
	Point dropDownScrollViewPos;
	std::unordered_map<WidgetId, ScrollViewState> scrollViewState;

	// Themes
	Theme* theme = nullptr;
	std::vector<Theme*> themes;
	std::unordered_map<WidgetType, std::string> widgetCurrentStyle;
	std::vector<std::pair<WidgetType, std::string>> widgetStyleStack;

	LayoutState layout;
	std::vector<LayoutState> layoutStack;
	Rect lastColumnRect;
	std::vector<Point> paddingStack[(i32)PaddingType::Count];
	std::vector<f32> spacingStack;
	std::vector<f32> columnSpacingStack;
	Point padding[(i32)PaddingType::Count] = { 0, 0 };
	f32 columnSpacing = 4;
	f32 spacing = 4;
	Point position = { 0, 0 };
	std::vector<Point> positionStack;

	// Table
	std::unordered_map<WidgetId, TablePersistentState> tablePersistentStates;
	std::vector<TableState> tableStack;

	Point cellPadding = { 2.0f, 2.0f };
	std::vector<Point> cellPaddingStack;

	// Tabbing/focusing
	TabIndex currentTabIndex = 0;
	TabIndex selectedTabIndex = 0;
	u32 disabledNesting = 0;
	DockTabGroupState tabGroup;

	DropdownState dropdown;
	ComboSliderState comboSlider;
	VectorEditorState vecEditor;
	RotarySliderState rotarySlider;
	SliderState slider;
	ColorPickerState colorPickerState;
	Rect tabGroupWidgetRect;

	// Input
	InputEvent event;
	std::vector<InputEvent> events;
	InputEvent::Type savedEventType = InputEvent::Type::None;
	std::vector<HNativeWindow> nativeWindows;

	// Colors, fonts and styles
	TintState tint;
	std::vector<TintState> tintStack;
	LineStyle lineStyle;
	FillStyle fillStyle;
	std::vector<HFont> fontStack;

	std::vector<u32> drawCmdIndexStack;
	std::vector<WidgetId> focusableWidgets;
	std::vector<WidgetId> lastFrameFocusableWidgets;

	// Mouse
	MouseCursorType mouseCursor = MouseCursorType::Arrow;
	HMouseCursor customMouseCursor = 0;
	bool mouseMoved = false;
	bool alreadyClickedOnSomething = false;

	DragDropState dragDrop;
	DockingState docking;

	Context()
	{
		popupStack.resize(maxPopupIndex);
		menuStack.resize(maxMenuDepth);
		idStack.push_back(0);
	}

	~Context();

	inline bool isActiveLayer() const
	{
		return maxLayerIndex == layerIndex;
	}

	void setLabelAndId(const char* text);
	void setSkipRenderAndInput(bool skip);
};

/// the current context, used internally
extern Context* ctx;

}