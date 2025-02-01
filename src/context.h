#pragma once
#include "types.h"
#include "text_input_state.h"

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

	// Various providers and singletons
	ServiceProviders* providers = nullptr;
	Renderer* renderer = nullptr;
	UnicodeTextCache* textCache = nullptr;
	Settings settings;

	// Global general state
	f32 deltaTime = 0;
	f32 totalTime = 0;
	u32 frameCount = 0;
	f32 pruneUnusedTextTime = 0; //TODO: maybe make it frames
	bool mustRedraw = false;
	bool focusChanged = false;
	bool skipRenderAndInput = false;
	bool hoveringThisWindow = false;
	Window* currentWindow = nullptr;
	WindowFlags nextWindowFlags = WindowFlags::None;
	HNativeWindow lastHoveredNativeWindow = nullptr;
	f32 scale = 1.0f;
	u32 atlasTextureSize = 4096;
	Point mousePosition;

	// Widgets
	WidgetId id = 42;
	WidgetState widget;
	TextInputState textInput;
	std::vector<TextLineState> textLines;
	std::vector<WidgetId> idStack;
	std::unordered_map<WidgetId, WidgetBoolState> widgetBools;

	// Same line
	bool sameLine = false;
	size_t sameLineInfoIndex = 0;
	size_t sameLineInfoCount = 0;
	f32 sameLineSpacing = 0;
	f32 sameLineHeight = 0;
	std::vector<f32> sameLineWidthStack;
	std::vector<f32> sameLineSpacingStack;
	std::vector<u32> sameLineInfoIndexStack;
	std::vector<bool> sameLineStack;
	SameLineState sameLineInfo[maxSameLineInfoIndex];

	// Toolbars
	std::vector<ToolbarState> toolbarStack;
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
	f32 menuIconSpace = 18;
	f32 menuFillerWidth = 30;

	// Scrolling
	ScrollViewState scrollViewStack[maxNestingIndex];
	f32 scrollViewSpeed = 0.2f;
	f32 scrollViewScrollPageSize = 0.4f;
	size_t scrollViewDepth = 0;
	WidgetId dragScrollViewHandleWidgetId = 0;
	f32 dropDownScrollViewPos = 0;

	// Themes
	Theme* theme = nullptr;
	std::vector<Theme*> themes;

	LayoutState layout;
	std::vector<LayoutState> layoutStack;
	Rect lastColumnRect;
	std::vector<Point> paddingStack;
	std::vector<f32> spacingStack;
	std::vector<f32> columnSpacingStack;
	Point padding = { 0, 0 };
	f32 columnSpacing = 4;
	f32 spacing = 4;
	Point position = { 0, 0 };
	std::vector<Point> positionStack;

	// Tabbing/focusing
	TabIndex currentTabIndex = 0;
	TabIndex selectedTabIndex = 0;
	DockTabGroupState tabGroup;

	DropdownState dropdown;
	ComboSliderState comboSlider;
	VectorEditorState vecEditor;
	RotarySliderState rotarySlider;
	SliderState slider;
	Rect tabGroupWidgetRect;

	// Input
	InputEvent event;
	std::vector<InputEvent> events;
	InputEvent::Type savedEventType = InputEvent::Type::None;
	std::vector<HNativeWindow> nativeWindows;

	// Colors and styles
	TintState tint;
	std::vector<TintState> tintStack;
	LineStyle lineStyle;
	FillStyle fillStyle;

	std::vector<u32> drawCmdIndexStack;

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

	void initializeRenderer();

	inline bool isActiveLayer() const
	{
		return maxLayerIndex == layerIndex;
	}

	void extractLabelAndId(const char* text);

	void setSkipRenderAndInput(bool skip);

	Rect drawMultilineText(
		const char* text,
		const Rect& rect,
		HAlignType horizontal = HAlignType::Left,
		VAlignType vertical = VAlignType::Top);
};

/// the current context, used internally
extern Context* ctx;

}