#pragma once
#include "horus.h"
#include "horus_interfaces.h"
#include "types.h"
#include "text_input_state.h"
#include <string>
#include <unordered_map>

namespace hui
{
struct Context
{
	struct TextLine
	{
		u32 start = 0;
		u32 length = 0;
	};

	struct SameLineInfo
	{
		bool computeHeight = true;
		f32 lineHeight = 0;
		f32 lineY = 0;
	};

	struct ToolbarState
	{
		ToolbarDirection direction;
	};

	struct WidgetLoopInfo
	{
		u32 previousId;
		u32 startId;
		u32 maxCount;
	};

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
	ContextSettings settings;

	// Global general state
	f32 deltaTime = 0;
	f32 totalTime = 0;
	u32 frameCount = 0;
	f32 pruneUnusedTextTime = 0;
	u32 currentWindowIndex = 0;
	u32 currentWidgetId = 1;
	std::vector<WidgetLoopInfo> widgetLoopStack;
	u32 maxWidgetId = 0;
	bool mustRedraw = false;
	bool focusChanged = false;
	bool skipRenderAndInput = false;
	Window* currentWindow = nullptr;
	bool hoveringThisWindow = false;
	bool dockingTabPane = false;
	f32 globalScale = 1.0f;
	u32 atlasTextureSize = 4096;

	// Vertical toolbars
	bool verticalToolbar = false;
	std::vector<bool> verticalToolbarStack;

	// Widgets
	TextInputState textInput;
	std::vector<TextLine> textLines;
	WidgetState widget;
	std::vector<f32> sameLineWidthStack;
	std::vector<f32> sameLineSpacingStack;
	std::vector<u32> sameLineInfoIndexStack;
	std::vector<bool> sameLineStack;
	SameLineInfo sameLineInfo[maxSameLineInfoIndex];
	u32 sameLineInfoIndex = 0;
	u32 sameLineInfoCount = 0;
	std::vector<ToolbarState> toolbarStack;
	TooltipState tooltip;

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
	u32 activeMenuBarItemWidgetId = 0;
	bool contextMenuActive = false;
	bool contextMenuClicked = false;
	u32 contextMenuWidgetId = 0;
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
	u32 dragScrollViewHandleWidgetId = 0;
	f32 dropDownScrollViewPos = 0;

	// Themes
	Theme* theme = nullptr;
	std::vector<Theme*> themes;

	// Containers
	Rect containerRect;
	Point penPosition;
	std::vector<LayoutState> layoutStack;
	Rect lastColumnRect;
	std::vector<f32> layoutPaddingStack;
	std::vector<f32> columnPaddingStack;
	std::vector<f32> spacingStack;
	std::vector<f32> columnSpacingStack;
	f32 layoutPadding = 10;
	f32 columnPadding = 0;
	f32 columnSpacing = 4;
	f32 spacing = 4;
	std::vector<Point> penStack;

	// Tabbing/focusing
	TabIndex currentTabIndex = 0;
	TabIndex selectedTabIndex = 0;
	DockPaneTabGroupState paneGroupState;

	DropdownState dropdownState;

	// Input
	InputEvent event;
	std::vector<InputEvent> events;
	InputEvent::Type savedEventType = InputEvent::Type::None;
	std::vector<HOsWindow> osWindows;

	// Colors and styles
	std::unordered_map<u32, std::vector<Color>> tintStack;
	Color tint[(u32)TintColorType::Count] = { Color::white, Color::white };
	LineStyle lineStyle;
	FillStyle fillStyle;

	std::vector<u32> drawCmdIndexStack;

	// Mouse
	MouseCursorType mouseCursor = MouseCursorType::Arrow;
	HMouseCursor customMouseCursor = 0;
	bool mouseMoved = false;
	bool alreadyClickedOnSomething = false;
	Point oldMousePos = { 0, 0 };

	DragDropState dragDropState;
	DockingState dockingState;

	Context()
	{
		popupStack.resize(maxPopupIndex);
		menuStack.resize(maxMenuDepth);
	}

	void initializeGraphics();

	inline bool isActiveLayer() const
	{
		return maxLayerIndex == layerIndex;
	}

	inline const Color& getTint(TintColorType type) const
	{
		return tint[(int)type];
	}

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