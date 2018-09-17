#pragma once
#include "horus.h"
#include <vector>
#include <unordered_map>

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
class UiTheme;
class UnicodeTextCache;
class FontCache;
class UiFont;
struct UiImage;

typedef u32 UiImageId;
typedef u32 GlyphCode;
typedef std::vector<GlyphCode> UnicodeString;

struct UiThemeElement
{
	struct State
	{
		UiFont* font = nullptr;
		Color color = Color::white;
		Color textColor = Color::white;
		u32 border = 0;
		UiImage* image = nullptr;
		f32 width = 0;
		f32 height = 0;
	};

	struct StyleStates
	{
		State states[(u32)WidgetStateType::Count];
	};

	std::unordered_map<std::string, StyleStates> styles;
	StyleStates* currentStyle = nullptr;

	inline void setDefaultStyle() { currentStyle = &styles["default"]; }
	inline void setStyle(const char* styleName) { currentStyle = &styles[styleName]; }
	inline State& getState(WidgetStateType stateType) { return currentStyle->states[(u32)stateType]; }
	inline State& normalState() { return currentStyle->states[(u32)WidgetStateType::Normal]; }
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
	Point position = { 0, 0 };
	Point savedPenPosition = { 0, 0 };
	f32 width = 0;
	f32 height = 0;
	f32 maxPenPositionY = -10000000000;
	UiThemeElement::State* themeWidgetElementState = nullptr;
	Color themeElementColorTint;
};

struct WidgetState
{
	u32 focusedWidgetId = 0;
	u32 prevFocusableWidgetId = 0;
	u32 nextFocusableWidgetId = 0;
	u32 hoveredWidgetId = 0;
	bool enabled = true;
	bool pressed = false;
	bool visible = true;
	bool focusedWidgetPressed = false;
	bool clicked = false;
	bool hovered = false;
	bool focused = false;
	Rect rect;
	Rect hoveredWidgetRect;
	Rect focusedWidgetRect;
	WidgetType hoveredWidgetType = WidgetType::None;
};

struct MenuWidgetState
{
	bool isSubMenu = false;
	Point startPosition;
	Rect lastItemRect;
	bool active = false;
	bool activatedNow = false;
	u32 widgetId = 0;
	size_t hoveredItemIndex = ~0;
	size_t selectedItemIndex = ~0;
	size_t itemCount = 0;
	Point size;
};

struct PopupState
{
	f32 width = 0;
	f32 height = 0;
	u32 widgetId = 0;
	PopupPositionMode positionMode = PopupPositionMode::WindowCenter;
	Point position;
	Point moveOffset;
	WidgetElementId widgetElementId = WidgetElementId::PopupBody;
	bool alreadyClickedOnSomething = false;
	bool alreadyClosedWithEscape = false;
	bool active = false;
	bool incrementLayer = true;
	bool topMost = false;
	i32 oldZOrder = 0;
	bool startedToDrag = false;
	bool draggingPopup = false;
	bool opened = false;
	Window ownerWindow = 0;
	Point dragDelta, lastMouseDownPoint;
	Point lastMousePoint;
	Rect prevContainerRect;
};

struct TooltipState
{
	Point position;
	f32 timer = 0;
	f32 delayToShow = 1.0f;
	u32 widgetId = 0;
	bool show = false;
	f32 offsetFromCursor = 18.0f;
};

struct ScrollViewState
{
	bool draggingThumb = false;
	Point dragDelta;
	u32 widgetId = 0;
	f32 size = 0.0f;
	f32 virtualHeight = 0.0f;
	f32 scrollPosition = 0.0f;
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

struct DockPaneTabGroupState
{
	f32 tabWidth = 90;//TODO: externalize
	bool forceSqueezeTabs = false;
	f32 forceTabWidth = 0.f;
	f32 sideSpacing = 20;
};

struct DropdownState
{
	bool active = false;
	u32 widgetId = 0;
};

struct DockingSystemData
{
	std::vector<struct UiViewContainer*> viewContainers;
	struct UiViewContainer* currentViewContainer = nullptr;
	bool closeWindow = false;
	bool allowUndocking = true;
};

struct DragDropState
{
	bool draggingIntent = false;
	bool dragging = false;
	bool begunDragging = false;
	bool dropped = false;
	bool allowDrop = false;
	bool foundDropTarget = false;
	u32 widgetId = 0;
	Point lastMousePos;
	MouseCursor dropAllowedCursor = 0;
	void* dragObject = nullptr;
	u32 dragObjectType = 0;
};

struct VirtualListContentState
{
	Point lastPenPosition;
	u32 totalRowCount;
	f32 itemHeight;
};

extern DockingSystemData dockingData;

}
