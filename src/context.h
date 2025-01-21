#pragma once
#include "types.h"
#include "text_input_state.h"

namespace hui
{
struct Context
{
	static const int maxLayerCount = 256;
	static const int maxNestingIndex = 256;
	static const int maxPopupIndex = 256;
	static const int maxMenuDepth = 256;
	static const int maxBoxDepth = 256;
	static const int maxSameLineInfoIndex = 256;

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
	WidgetId currentWidgetId = 0;
	bool mustRedraw = false;
	bool focusChanged = false;
	bool skipRenderAndInput = false;
	Window* currentWindow = nullptr;
	WindowFlags nextWindowFlags = WindowFlags::None;
	HNativeWindow lastHoveredNativeWindow = nullptr;
	bool hoveringThisWindow = false;
	f32 scale = 1.0f;
	u32 atlasTextureSize = 4096;
	Point mousePosition;

	// Vertical toolbars
	bool verticalToolbar = false;
	std::vector<bool> verticalToolbarStack;

	// Widgets
	TextInputState textInput;
	std::vector<TextLineState> textLines;
	WidgetState widget;
	WidgetId id = 42;
	std::vector<WidgetId> idStack;
	std::unordered_map<WidgetId, WidgetBoolState> widgetBoolState;
	std::vector<f32> sameLineWidthStack;
	std::vector<f32> sameLineSpacingStack;
	std::vector<u32> sameLineInfoIndexStack;
	std::vector<bool> sameLineStack;
	SameLineState sameLineInfo[maxSameLineInfoIndex];
	u32 sameLineInfoIndex = 0;
	u32 sameLineInfoCount = 0;
	std::vector<ToolbarState> toolbarStack;
	TooltipState tooltip;
	std::string widgetLabel;

	u32 layerIndex = 0;
	u32 maxLayerIndex = 0;

	// Popups
	std::vector<PopupState> popupStack;
	bool popupUseGlobalScale = true;
	u32 popupIndex = 0;

	// Virtual list
	std::vector<VirtualListContentState> virtualListStack;

	// Menus
	std::vector<MenuWidgetState> menuStack;
	u32 menuDepth = 0;
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
	u32 hoveredSimpleMenuItemMenuDepth = ~0;
	u32 activeMenuBarId = 0;
	u32 currentMenuBarId = 0;
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
	std::vector<f32> paddingStack;
	std::vector<f32> spacingStack;
	std::vector<f32> columnSpacingStack;
	f32 padding = 10;
	f32 columnSpacing = 4;
	f32 spacing = 4;
	Point position = { 0, 0 };
	std::vector<Point> positionStack;

	// Tabbing/focusing
	TabIndex currentTabIndex = 0;
	TabIndex selectedTabIndex = 0;
	DockPaneTabGroupState paneGroupState;

	DropdownState dropdownState;

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

	DragDropState dragDropState;
	DockingState dockingState;

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

	void extractLabelAndId(const char* text, std::string& label, WidgetId& id);

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