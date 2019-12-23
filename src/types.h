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

	struct Style
	{
		State states[(u32)WidgetStateType::Count];
		std::unordered_map<std::string, std::string> parameters;
		std::unordered_map<std::string, f32> floatParameters;
		std::unordered_map<std::string, Color> colorParameters;

		f32 getParameterValue(const std::string& name, f32 defaultValue)
		{
			auto iter = floatParameters.find(name);

			if (iter == floatParameters.end())
			{
				auto iter2 = parameters.find(name);

				if (iter2 != parameters.end())
				{
					f32 val = atof(iter2->second.c_str());
					floatParameters[name] = val;
					return val;
				}

				return defaultValue;
			}
			
			return iter->second;
		}

		Color getParameterValue(const std::string& name, const Color& defaultValue = Color::white)
		{
			auto iter = colorParameters.find(name);

			if (iter == colorParameters.end())
			{
				auto iter2 = parameters.find(name);

				if (iter2 != parameters.end())
				{
					Color c;
					getColorFromText(iter2->second.c_str(), c);
					colorParameters[name] = c;
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
	inline void setStyle(const char* styleName) { currentStyle = &styles[styleName]; }
	inline State& getState(WidgetStateType stateType) { return currentStyle->states[(u32)stateType]; }
	inline State& normalState() { return currentStyle->states[(u32)WidgetStateType::Normal]; }
	inline State& focusedState() { return currentStyle->states[(u32)WidgetStateType::Focused]; }
	inline State& pressedState() { return currentStyle->states[(u32)WidgetStateType::Pressed]; }
	inline State& hoveredState() { return currentStyle->states[(u32)WidgetStateType::Hovered]; }
	inline State& disabledState() { return currentStyle->states[(u32)WidgetStateType::Disabled]; }
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
	bool savedSameLine = false;
	f32 savedHighestSameLineY;
	f32 savedPreviousSameLineY;
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
	f32 sameLineSpacing = 0;
	f32 sameLineHeight = 0;
	bool sameLine = false;
	f32 width = 0; // if 0 then it will be automatically computed, usually the parent container width
	bool enabled = true;
	bool pressed = false;
	bool visible = true;
	bool focusedWidgetPressed = false;
	bool clicked = false;
	bool hovered = false;
	bool focused = false;
	bool changeEnded = false;
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
	bool closeTooltipPopup = false;
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
