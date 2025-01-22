#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "horus.h"

#ifdef _LINUX
#include <sys/types.h>
#include <cstddef>
#include <stddef.h>
#include <stdint.h>
#include <locale.h>
#include <stdlib.h>
#endif

namespace hui
{
class Renderer;
struct TextureArray;
class Theme;
class UnicodeTextCache;
class FontCache;
class Font;
struct Image;
struct DockNode;

typedef u32 ImageId;

struct TextLineState
{
	u32 start = 0;
	u32 length = 0;
};

struct SameLineState
{
	bool computeHeight = true;
	f32 lineHeight = 0;
	f32 lineY = 0;
};

struct ToolbarState
{
	ToolbarDirection direction = ToolbarDirection::Horizontal;
};

struct TintState
{
	Color color[(i32)TintColorType::Count];
	TintColorOpType op[(i32)TintColorType::Count] = { TintColorOpType::None };
};

struct WidgetBoolState
{
	u32 lastUsedFrame = 0;
	bool value = false;
};

struct ComboSliderState
{
	static const size_t maxTextSize = 128;

	bool mouseWasDown = false;
	bool dragging = false;
	bool editingText = false;
	Point dragLastMousePos;
	char text[maxTextSize] = {0};
	WidgetId newId = 0;
	WidgetId id = 0;
	bool requestChangeToOtherComboSlider = false;
};

struct ThemeElement
{
	struct State
	{
		Font* font = nullptr;
		Color color = Color::white;
		Color textColor = Color::white;
		u32 border = 0;
		Image* image = nullptr;
		f32 width = 0;
		f32 height = 0;
	};

	struct Style
	{
		State states[(u32)WidgetStateType::Count];
		std::unordered_map<std::string, std::string> parameters;
		std::unordered_map<std::string, f32> cachedFloatParameters;
		std::unordered_map<std::string, Color> cachedColorParameters;

		f32 getParameterValue(const std::string& name, f32 defaultValue)
		{
			auto iter = cachedFloatParameters.find(name);

			// if not found, search it in the parameters and cache the value
			if (iter == cachedFloatParameters.end())
			{
				auto iter2 = parameters.find(name);

				if (iter2 != parameters.end())
				{
					f32 val = atof(iter2->second.c_str());
					cachedFloatParameters[name] = val;
					return val;
				}

				return defaultValue;
			}

			return iter->second;
		}

		Color getParameterValue(const std::string& name, const Color& defaultValue = Color::white)
		{
			auto iter = cachedColorParameters.find(name);

			// if not found, search it in the parameters and cache the value
			if (iter == cachedColorParameters.end())
			{
				auto iter2 = parameters.find(name);

				if (iter2 != parameters.end())
				{
					Color c = getColorFromText(iter2->second.c_str());
					cachedColorParameters[name] = c;
					return c;
				}

				return defaultValue;
			}

			return iter->second;
		}
	};

	std::unordered_map<std::string, Style> styles;
	Style* currentStyle = nullptr;

	inline void setDefaultStyle() { currentStyle = &styles["default"]; }
	inline void setStyle(const char* styleName) 
	{
		auto iter = styles.find(styleName);

		if (iter != styles.end())
			currentStyle = &iter->second;
		else
		{
			// just set the first one if there is a style in the list
			if (!currentStyle)
				if (!styles.empty()) currentStyle = &styles.begin()->second;
		}
	}
	inline State& getState(WidgetStateType stateType) { return currentStyle->states[(u32)stateType]; }
	inline State& normalState() const { return currentStyle->states[(u32)WidgetStateType::Normal]; }
	inline State& focusedState() const { return currentStyle->states[(u32)WidgetStateType::Focused]; }
	inline State& pressedState() const { return currentStyle->states[(u32)WidgetStateType::Pressed]; }
	inline State& hoveredState() const { return currentStyle->states[(u32)WidgetStateType::Hovered]; }
	inline State& disabledState() const { return currentStyle->states[(u32)WidgetStateType::Disabled]; }
	inline State& getStyleState(const char* styleName, WidgetStateType stateType) { return styles[styleName].states[(u32)stateType]; }
	inline State& styleNormalState(const char* styleName) { return styles[styleName].states[(u32)WidgetStateType::Normal]; }
};

enum class LayoutType
{
	Container,
	Vertical,
	Columns,
	Column,
	ScrollView
};

struct LayoutState
{
	LayoutState() {}

	LayoutState(LayoutType newType)
		: type(newType)
	{}

	LayoutType type = LayoutType::Vertical;
	i32 currentColumn = 0;
	std::vector<f32> columnSizes;
	std::vector<f32> columnMinSizes;
	std::vector<f32> columnMaxSizes;
	std::vector<f32> columnPixelSizes;
	Point savedPosition = { 0, 0 };
	Point columnsPosition = { 0, 0 };
	bool savedSameLine = false;
	f32 savedHighestSameLineY = 0;
	f32 savedPreviousSameLineY = 0;
	f32 width = 0;
	f32 height = 0;
	f32 maxPositionY = -10000000000;
	ThemeElement::State* themeWidgetElementState = nullptr;
	Color themeElementColorTint;
};

struct WidgetState
{
	WidgetId focusedId = 0;
	WidgetId prevFocusableId = 0;
	WidgetId nextFocusableId = 0;
	WidgetId hoveredId = 0;
	f32 width = 0; // if 0 then it will be automatically computed, usually the parent container width
	bool disabled = false;
	bool pressed = false;
	bool visible = true;
	bool clicked = false;
	bool hovered = false;
	bool focused = false;
	bool changeEnded = false;
	bool focusedAndPressed = false;
	Rect rect;
	Rect hoveredWidgetRect;
	Rect focusedWidgetRect;
	WidgetType hoveredType = WidgetType::None;
};

struct MenuWidgetState
{
	bool isSubMenu = false;
	Point startPosition;
	Rect lastItemRect;
	bool active = false;
	bool activatedNow = false;
	WidgetId widgetId = 0;
	size_t hoveredItemIndex = ~0;
	size_t selectedItemIndex = ~0;
	size_t itemCount = 0;
	Point size;
};

struct PopupState
{
	f32 width = 0;
	f32 height = 0;
	WidgetId widgetId = 0;
	PopupFlags flags = PopupFlags::None;
	Point position;
	Point moveOffset;
	WidgetElementId widgetElementId = WidgetElementId::PopupBody;
	bool alreadyClickedOnSomething = false;
	bool alreadyClosedWithEscape = false;
	bool active = false;
	i32 oldZOrder = 0;
	bool startedToDrag = false;
	bool draggingPopup = false;
	bool opened = false;
	HNativeWindow ownerWindow = 0;
	Point dragDelta, lastMouseDownPoint;
	Point lastMousePoint;
	Rect prevContainerRect;
};

struct TooltipState
{
	Point position;
	f32 timer = 0;
	f32 delayToShow = 1.0f;
	WidgetId widgetId = 0;
	bool show = false;
	bool closeTooltipPopup = false;
	f32 offsetFromCursor = 18.0f;
};

struct ScrollViewState
{
	bool draggingThumb = false;
	Point dragDelta;
	WidgetId widgetId = 0;
	f32 size = 0.0f;
	f32 virtualHeight = 0.0f;
	f32 scrollPosition = 0.0f;
	Rect rect;
};

struct TextMarker
{
	enum class Type
	{
		Normal,
		Color,
		Underline,
		Bold,
		Italic,
		BoldItalic,
		Link,
		BreakLine
	};

	TextMarker() {}
	Type type = Type::Normal;
	u32 start = 0;
	bool isEnd = false;
	u32 userData = 0;
	f32 x = 0, y = 0;
	u32 line = 0;
};

struct DockTabGroupState
{
	f32 tabWidth = 90;//TODO: externalize
	bool forceSqueezeTabs = false;
	f32 forceTabWidth = 0.f;
	f32 sideSpacing = 20;
};

struct DropdownState
{
	bool active = false;
	WidgetId widgetId = 0;
};

struct Window
{
	struct DockNode* dockNode = nullptr;
	std::string id, title;
	HImage icon = 0;
	Rect tabRect, clientRect;
	bool dockingNow = false;
};

struct DockingState
{
	~DockingState();
	std::unordered_map<HNativeWindow, struct DockNode*> rootNativeWindowDockNodes;
	std::unordered_map<std::string, Window*> windows;
	std::unordered_set<Window*> windowsToDelete;
	std::unordered_set<DockNode*> dockNodesToDelete;
	std::unordered_set<HNativeWindow> nativeWindowsToDelete;
	std::unordered_map<std::string /*window name*/, Rect> closedWindowsRects;
	std::unordered_map<std::string /*window name*/, DockNodeId> windowsDockNodeAssignments;
	std::unordered_map<DockNodeId, DockNode*> dockNodeIdsMap;
	DockNode* currentDockNode = nullptr;
	Window* focusedWindow = nullptr;
	bool closeWindow = false;
	bool dragStarted = false;
	HNativeWindow dragIndicatorNativeWindow = nullptr;
	DockNode* resizingNode = nullptr;
	DockNode* lastHoveredNode = nullptr;
	DockNode* hoveredNode = nullptr;
	DockType dockType = DockType::None;
	DockNode* dockToNode = nullptr;
	Point lastMousePosSinceMouseDown;
	Point lastMousePos;
	Point mouseDragDelta;
	Rect dragRect;
	bool drawingWindowTabs = false;
	Window* dragWindow = nullptr;
	Point dragWindowMouseDelta;
	DockNodeId nextDockNodeId = 1;
	Rect hitBoxLeft;
	Rect hitBoxRight;
	Rect hitBoxTop;
	Rect hitBoxBottom;
	Rect hitBoxTabs;
	Rect hitBoxTabsBar;
	Rect hitBoxRootLeft;
	Rect hitBoxRootRight;
	Rect hitBoxRootTop;
	Rect hitBoxRootBottom;
	bool isHitBoxLeftHovered = false;
	bool isHitBoxRightHovered = false;
	bool isHitBoxTopHovered = false;
	bool isHitBoxBottomHovered = false;
	bool isHitBoxTabsHovered = false;
	bool isHitBoxTabsBarHovered = false;
	bool isHitBoxRootLeftHovered = false;
	bool isHitBoxRootRightHovered = false;
	bool isHitBoxRootTopHovered = false;
	bool isHitBoxRootBottomHovered = false;
};

struct DragDropState
{
	bool draggingIntent = false;
	bool dragging = false;
	bool begunDragging = false;
	bool dropped = false;
	bool allowDrop = false;
	bool foundDropTarget = false;
	WidgetId widgetId = 0;
	Point lastMousePos;
	HMouseCursor dropAllowedCursor = 0;
	void* dragObject = nullptr;
	u32 dragObjectType = 0;
};

struct VirtualListContentState
{
	Point lastPenPosition;
	u32 totalRowCount = 0;
	f32 itemHeight = 0;
};

struct MemoryStream
{
	enum class Mode
	{
		None,
		Read,
		Write
	};

	Mode currentMode = Mode::None;
	std::vector<u8> data;
	size_t currentOffset = 0;

	bool beginWrite();
	bool beginRead(const u8* data, size_t dataSize);
	void writeData(const u8* data, size_t dataSize);
	bool readData(u8* outData, size_t dataSize);
	template <typename Type> void write(const Type* data, size_t dataSize)
	{ writeData((u8*)data, dataSize); }
	template <typename Type> bool read(Type* data, size_t dataSize)
	{ return readData((u8*)data, dataSize); }
};

}
