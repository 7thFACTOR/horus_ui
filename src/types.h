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
struct Renderer;
struct TextureArray;
struct Theme;
struct UnicodeTextCache;
struct FontCache;
struct Font;
struct Image;
struct DockNode;

typedef u32 ImageId;
typedef std::vector<struct DrawCommand> DrawCommandVector;

/// How an image is drawn, repeated or stretched across the rectangle
enum class ImageSizingPolicy
{
	Stretch,
	Repeat
};

/// Text styling info
struct TextStyle
{
	Rgba32 backFillColor; /// the text color
	bool underline = false; /// true if underline
	bool backFill = false; /// true if back is filled color
};

struct DrawCmdLayerSplitter
{
	std::vector<DrawCommandVector> layers;
	size_t currentLayerIndex = 0;

	DrawCmdLayerSplitter();
	~DrawCmdLayerSplitter() {}
	void clear();
	void split(u32 layerCount);
	void merge();
	void setLayer(u32 index);
};

struct DrawCommand
{
	enum class Type
	{
		None,
		DrawRect,
		DrawQuad,
		DrawQuad4Colors,
		DrawImageBordered,
		DrawLine,
		DrawPolyLine,
		DrawText,
		DrawSolidTriangle,
		ClipRect,
		SetViewportOffset,
		SetAtlas,
		SetColor,
		SetFont,
		SetTextStyle,
		SetLineStyle,
		SetFillStyle,
		ClearBackground,
		Callback,

		Count
	};

	struct CmdDrawRect
	{
		Rect rect;
		Rect uvRect;
		bool rotated;
		u32 textureIndex;
		bool wire = false;
	};

	struct CmdDrawQuad
	{
		Point corners[4];
		Image* image = nullptr;
	};

	struct CmdDrawTriangle
	{
		Point p1, p2, p3;
		Point uv1, uv2, uv3;
		Rgba32 c1, c2, c3;
		Image* image = nullptr;
	};

	struct CmdDrawLine
	{
		Point a, b;
	};

	struct CmdDrawPolyLine
	{
		Point* points;
		u32 count;
		bool closed;
	};

	struct CmdDrawText
	{
		Rect rect;
		HAlignType horizAlign;
		VAlignType vertAlign;
		char* text;
		bool singleLineEllipsis;
		bool noWordWrap;
	};

	struct CmdDrawImageBordered
	{
		Rect rect;
		Image* image;
		f32 border;
		f32 scale;
	};

	struct CmdDrawQuad4Colors
	{
		Rect rect;
		Rect uvRect;
		Image* image = nullptr;
		Rgba32 topLeft;
		Rgba32 topRight;
		Rgba32 bottomLeft;
		Rgba32 bottomRight;
	};

	struct CmdSetViewportOffset
	{
		Point offset;
	};

	DrawCommand() {}
	DrawCommand(Type newType)
		: type(newType)
	{
	}

	Type type = Type::None;

	union CmdData
	{
		CmdDrawRect drawRect;
		CmdDrawQuad drawQuad;
		CmdDrawLine drawLine;
		CmdDrawPolyLine drawPolyLine;
		CmdDrawText drawText;
		CmdDrawImageBordered drawImageBordered;
		CmdDrawQuad4Colors drawQuad4Colors;
		CmdDrawTriangle drawTriangle;
		CmdSetViewportOffset setViewportOffset;
		RenderCallback callback;
		Rect clipRect;
		bool clipToParent;
		bool popClipRect = false;
		struct Atlas* setAtlas;
		Rgba32 setColor;
		struct Font* setFont;
		Rgba32 setTextColor;
		TextStyle setTextStyle;
		LineStyle setLineStyle;
		FillStyle setFillStyle;
	} data;

	DrawCommand(const DrawCommand& other)
	{
		*this = other;
	}

	DrawCommand& operator = (const DrawCommand& other)
	{
		type = other.type;
		data.drawRect = other.data.drawRect;
		data.drawQuad = other.data.drawQuad;
		data.drawLine = other.data.drawLine;
		data.drawPolyLine = other.data.drawPolyLine;
		data.drawText = other.data.drawText;
		data.drawImageBordered = other.data.drawImageBordered;
		data.drawQuad4Colors = other.data.drawQuad4Colors;
		data.drawTriangle = other.data.drawTriangle;
		data.setViewportOffset = other.data.setViewportOffset;
		data.clipRect = other.data.clipRect;
		data.clipToParent = other.data.clipToParent;
		data.popClipRect = other.data.popClipRect;
		data.setAtlas = other.data.setAtlas;
		data.setColor = other.data.setColor;
		data.setFont = other.data.setFont;
		data.setTextColor = other.data.setTextColor;
		data.setTextStyle = other.data.setTextStyle;
		data.setLineStyle = other.data.setLineStyle;
		data.setFillStyle = other.data.setFillStyle;
		data.callback = other.data.callback;

		return *this;
	}
};

struct TextLineState
{
	u32 start = 0;
	u32 length = 0;
};

struct SameLineState
{
	bool enabled = false;
	bool wasEnabled = false;
	f32 spacing = 5;
	f32 maxHeight = 0;
	f32 currentY = 0;
	f32 currentX = 0;
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
	static const size_t maxTextSize = 1024;

	f32 currentValue = 0;
	bool mouseWasDown = false;
	bool dragging = false;
	bool editingText = false;
	bool clickedToEditText = false;
	Point dragLastMousePos;
	char text[maxTextSize] = {0};
	WidgetId newId = 0;
	WidgetId id = 0;
	bool requestChangeToOtherComboSlider = false;
};

struct VectorEditorState
{
	bool draggingValue = false;
	Point lastMousePos;
	WidgetId draggedId = 0;
	static const size_t maxStrSize = 50;
	char strX[maxStrSize] = { 0 };
	char strY[maxStrSize] = { 0 };
	char strZ[maxStrSize] = { 0 };
	f32 colWidthsPRS[6];

	VectorEditorState()
	{
		colWidthsPRS[0] = 14;
		colWidthsPRS[1] = -1;
		colWidthsPRS[2] = 14;
		colWidthsPRS[3] = -1;
		colWidthsPRS[4] = 14;
		colWidthsPRS[5] = -1;
	};
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

		f32 getParameter(const std::string& name, f32 defaultValue)
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

		Color getColorParameter(const std::string& name, const Color& defaultValue = Color::white)
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
	Generic,
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
	bool firstWidgetInLayout = true;
	WidgetId id = 0;
	Point savedPosition = { 0, 0 };
	SameLineState savedSameLine;
	f32 width = 0;
	f32 height = 0;
};

struct WidgetState
{
	WidgetId focusedId = 0;
	WidgetId prevFocusableId = 0;
	WidgetId nextFocusableId = 0;
	WidgetId hoveredId = 0;
	WidgetId captureId = 0;
	f32 width = 0; // if 0 then it will be automatically computed, usually the parent container width
	f32 nextWidth = 0; // if set, will be applied to the next widget
	f32 customWidth = 0;
	bool hasNextWidth = false; // whether nextWidth is set
	bool hasCustomWidth = false;
	bool disabled = false;
	bool pressed = false;
	bool visible = true;
	bool clicked = false;
	bool hovered = false;
	bool focused = false;
	bool changeEnded = false;
	bool nextDisabled = false; // whether the next widget will be disabled
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
	WidgetId id = 0;
	size_t hoveredItemIndex = ~0;
	size_t selectedItemIndex = ~0;
	size_t itemCount = 0;
	Point size;
};

struct TableColumnState
{
	f32 width = 100.0f;
	f32 specifiedSize = 0.0f; // User-specified size (percentage or pixels)
	f32 minWidth = 10.0f;
	f32 maxWidth = 10000.0f;
	bool isResizable = false; // Derived from flags usually, but can be explicit
	bool isPercentage = false; // If true, specifiedSize is 0..1 percentage
	bool isFillRemaining = false; // If true, this column fills remaining space
	bool isHidden = false;
	bool isStretchable = true; // Track if this column should participate in auto-stretch
	bool userResized = false; // Track if this column was manually resized by user
	TableColumnFlags flags = TableColumnFlags::None;
};

struct TablePersistentState
{
	TablePersistentState();
	~TablePersistentState();

	std::vector<TableColumnState> columns;
	bool initialized = false;

	// Resizing state
	bool resizingColumn = false;
	u32 resizingColumnIndex = ~0;
	f32 resizeStartX = 0;
	f32 resizeStartWidth = 0;
	f32 resizeStartWidthRight = 0;
	Point lastMousePos;
	struct DrawCmdLayerSplitter* splitter = nullptr;
	Point scrollViewScrollPos;
};

struct BoxState
{
	f32 width = 0.0f;
	ThemeElement::State* themeWidgetElementState = nullptr;
	Color themeElementColorTint;
	Point savedPadding;
};

struct TableState
{
	TablePersistentState* persistent = nullptr;
	u32 currentColumn = 0;
	WidgetId id = 0;
	Rect headerRect;
	Rect tableRect;
	f32 innerWidth = 0;
	f32 innerHeight = 0;
	// Resizing state is now in persistent state only
	// bool resizingColumn = false; // REMOVED
	// u32 resizingColumnIndex = ~0; // REMOVED
	// Point lastMousePos; // REMOVED (using persistent.lastMousePos)
	bool hasTableClip = false;

	// New fields for dynamic layout
	bool isInHeader = false;
	u32 currentRow = 0;
	f32 currentRowY = 0;
	Color currentRowColor;
	bool currentRowColorSet = false;
	Color currentCellColor;
	bool currentCellColorSet = false;
	TableFlags flags = TableFlags::None;
	f32 currentMaxRowHeight = 0;
	f32 rowStartY = 0;
	f32 cellStartY = 0;
	f32 rowHeight = 0;
	u32 rowDrawCmdIndex = 0;
	std::vector<f32> rowSeparators;
	struct CellColorRequest
	{
		u32 columnIndex;
		Color color;
	};
	std::vector<CellColorRequest> cellColorRequests;
	f32 bodyStartY = 0.0f;
	bool isClipping = false;
	f32 savedLayoutWidth = 0.0f;
	bool needsScrollViewStart = false;
	f32 scrollViewBaseX = 0.0f;
};

struct PopupState
{
	f32 width = 0;
	f32 height = 0;
	WidgetId id = 0;
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
	std::vector<u32> savedSameLineInfoIndexStack;
	SameLineState savedSameLine;
};

struct RotarySliderState
{
	Point lastMousePos;
	WidgetId id = 0;
	bool isFine = false;
};

struct TooltipState
{
	Point position;
	f32 timer = 0;
	f32 resetTimer = 10;
	f32 delayToShow = 1.0f;
	f32 delayToShowConsecutive = 0.5f;
	WidgetId id = 0;
	WidgetId lastId = 0;
	bool show = false;
	bool wasShown = false;
	bool closeTooltipPopup = false;
	f32 offsetFromCursor = 18.0f;
};

struct ScrollViewState
{
	bool draggingThumb = false;
	Point dragDelta;
	WidgetId id = 0;
	f32 size = 0.0f;
	f32 virtualHeight = 0.0f;
	f32 scrollPosition = 0.0f;
	Rect rect;
	ScrollViewFlags flags = ScrollViewFlags::None;
	
	// Horizontal scrolling support
	f32 virtualWidth = 0.0f;
	f32 scrollPositionX = 0.0f;
	bool draggingThumbX = false;
	Point dragDeltaX;
	f32 maxContentX = 0.0f; // Track rightmost position for content width
	bool wasHorizontalScrollbarVisible = false; // Track if H-bar was visible last frame to reserve space consistently
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
	WidgetId id = 0;
};

struct SliderState
{
	bool draggingKnob = false;
	Point dragDelta;
};

struct Window
{
	struct DockNode* dockNode = nullptr;
	std::string id, title;
	HImage image = 0;
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
	WidgetId id = 0;
	Point lastMousePos;
	HMouseCursor dropAllowedCursor = 0;
	void* dragObject = nullptr;
	u32 dragObjectType = 0;
};

struct VirtualListContentState
{
	Point lastPosition;
	u32 totalRowCount = 0;
	f32 itemHeight = 0;
};

struct ColorPickerState
{
	u32 draggingElementId = ~0;
	Color currentHsv;
	Color currentRgb;
	Color oldColor;
	i32 intR, intG, intB, intA;
	static const u32 maxHexColorSize = 9;
	char hexColor[maxHexColorSize] = {0};
	WidgetId currentEditingId = 0;
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
