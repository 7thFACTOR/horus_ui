#pragma once
#include "horus.h"
#include "horus_interfaces.h"
#include "types.h"
#include "text_input_state.h"
#include <string>
#include <unordered_map>

namespace hui
{
struct UiContext
{
	struct TextLine
	{
		u32 start = 0;
		u32 length = 0;
	};

	static const int maxLayerCount = 256;
	static const int maxNestingIndex = 256;
	static const int maxPopupIndex = 256;
	static const int maxMenuDepth = 256;
	static const int maxBoxDepth = 256;

	GraphicsProvider* gfx = nullptr;
	Renderer* renderer = nullptr;
	UnicodeTextCache* textCache = nullptr;
	ContextSettings settings;

	f32 deltaTime = 0;
	f32 totalTime = 0;
	u32 frameCount = 0;
	f32 pruneUnusedTextTime = 0;
	u32 currentWindowIndex = 0;
	GraphicsApiContext gfxApiContext = 0;
	u32 currentWidgetId = 1;
	std::vector<u32> widgetIdStack;
	u32 maxWidgetId = 0;
	bool mustRedraw = false;
	bool focusChanged = false;
	bool skipRenderAndInput = false;
	bool hoveringThisWindow = false;
	bool dockingTabPane = false;
	f32 globalScale = 1.0f;
	u32 atlasTextureSize = 4096;
	bool drawingViewPaneTabs = false;

	TextInputState textInput;
	WidgetState widget;
	TooltipState tooltip;

	u32 layerIndex = 0;
	u32 maxLayerIndex = 0;

	std::vector<PopupState> popupStack;
	bool popupUseGlobalScale = true;
	u32 popupIndex = 0;

	std::vector<VirtualListContentState> virtualListStack;

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

	ScrollViewState scrollViewStack[maxNestingIndex];
	f32 scrollViewSpeed = 0.2f;
	f32 scrollViewScrollPageSize = 0.4f;
	size_t scrollViewDepth = 0;
	u32 dragScrollViewHandleWidgetId = 0;
	f32 dropDownScrollViewPos = 0;

	UiTheme* theme = nullptr;
	std::vector<UiTheme*> themes;

	Rect containerRect;
	Point penPosition;
	std::vector<LayoutState> layoutStack;
	Rect lastColumnRect;
	std::vector<f32> paddingStack;
	std::vector<f32> spacingStack;
	f32 padding = 4;
	f32 spacing = 4;
	std::vector<Point> penStack;

	TabIndex currentTabIndex = 0;
	TabIndex selectedTabIndex = 0;
	DockPaneTabGroupState paneGroupState;

	DropdownState dropdownState;

	InputProvider* inputProvider = nullptr;
	InputEvent event;
	std::vector<InputEvent> events;
	InputEvent::Type savedEventType = InputEvent::Type::None;

	std::unordered_map<TintColorType, std::vector<Color>> tintStack;
	Color tint[(u32)TintColorType::Count] = { Color::white, Color::white };
	LineStyle lineStyle;
	FillStyle fillStyle;

	std::vector<u32> drawCmdIndexStack;

	MouseCursorType mouseCursor = MouseCursorType::Arrow;
	MouseCursor customMouseCursor = 0;
	bool mouseMoved = false;
	Point oldMousePos = { 0, 0 };

	DragDropState dragDropState;

	std::vector<TextLine> textLines;

	UiContext()
	{
		popupStack.resize(maxPopupIndex);
		menuStack.resize(maxMenuDepth);
	}

	void initializeGraphics();

	inline bool isActiveLayer() const
	{
		return maxLayerIndex == layerIndex;
	}

	void setSkipRenderAndInput(bool skip);

	Rect drawMultilineText(
		const char* text,
		const Rect& rect,
		HAlignType horizontal = HAlignType::Left,
		VAlignType vertical = VAlignType::Top);
};

//! the current context, used internally
extern UiContext* ctx;

}