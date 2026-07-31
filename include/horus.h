#pragma once
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <vector>
#include <string>

/*
------------------------------------------------------------------------------
	Horus UI
------------------------------------------------------------------------------
	Immediate Mode Graphical User Interface Library

	(C) All rights reserved 2016-2026 7thFACTOR Software - Nicusor Nedelcu (nekitu)
------------------------------------------------------------------------------
*/

#ifdef HUI_CUSTOM_CONFIG_FILE
#include HUI_CUSTOM_CONFIG_FILE
#endif

#ifndef HUI_NO_BASIC_TYPES
#ifndef HUI_NO_U8
typedef uint8_t u8;
#endif

#ifndef HUI_NO_U16
typedef uint16_t u16;
#endif

#ifndef HUI_NO_U32
typedef uint32_t u32;
#endif

#ifndef HUI_NO_U64
typedef uint64_t u64;
#endif

#ifndef HUI_NO_U128
typedef struct
{
	u64 data[2];
} u128;
#endif

#ifndef HUI_NO_I8
typedef int8_t i8;
#endif

#ifndef HUI_NO_I16
typedef int16_t i16;
#endif

#ifndef HUI_NO_I32
typedef int32_t i32;
#endif

#ifndef HUI_NO_I64
typedef int64_t i64;
#endif

#ifndef HUI_NO_I128
typedef struct
{
	i64 data[2];
} i128;
#endif

#ifndef HUI_NO_F32
typedef float f32;
#endif

#ifndef HUI_NO_F64
typedef double f64;
#endif

#endif

#ifdef HUI_STATIC
	#define HUI_API
	#define HUI_STRUCT_API
#else
#ifdef _WINDOWS
	#ifdef HUI_EXPORT
		#define HUI_API extern "C++" __declspec(dllexport)
		#define HUI_STRUCT_API __declspec(dllexport)
	#else
		#ifdef HUI_IMPORT
			#define HUI_API extern "C++" __declspec(dllimport)
			#define HUI_STRUCT_API __declspec(dllimport)
		#else
			#define HUI_API
			#define HUI_STRUCT_API
		#endif
	#endif
#else
	#ifdef HUI_EXPORT
		#define HUI_API __attribute__((dllexport))
		#define HUI_STRUCT_API __attribute__((dllexport))
	#else
		#ifdef HUI_IMPORT
			#define HUI_API __attribute__((dllimport))
			#define HUI_STRUCT_API __attribute__((dllimport))
		#else
			#define HUI_API
			#define HUI_STRUCT_API
		#endif
	#endif
#endif
#endif

#define HUI_SERVICES hui::contextGetSettings().services

namespace hui
{
#define HUI_BIT(bit) (1<<bit)
#define HUI_ENUM_AS_FLAGS(T)\
	HUI_ENUM_AS_FLAGS_EX(T, u32)
#define HUI_ENUM_AS_FLAGS_EX(T, enumBasicType) \
inline T operator & (T x, T y) { return static_cast<T> (static_cast<enumBasicType>(x) & static_cast<enumBasicType>(y)); }; \
inline T operator | (T x, T y) { return static_cast<T> (static_cast<enumBasicType>(x) | static_cast<enumBasicType>(y)); }; \
inline T operator ^ (T x, T y) { return static_cast<T> (static_cast<enumBasicType>(x) ^ static_cast<enumBasicType>(y)); }; \
inline T operator ~ (T x) { return static_cast<T> (~static_cast<enumBasicType>(x)); }; \
inline T& operator &= (T& x, T y) { x = x & y; return x; }; \
inline T& operator |= (T& x, T y) { x = x | y; return x; }; \
inline T& operator ^= (T& x, T y) { x = x ^ y; return x; }; \
inline bool operator !(T x) { return !(enumBasicType)(x); }; \
inline bool checkFlags(T x) { return (enumBasicType)x != 0; }; \
inline bool any(T x) { return (enumBasicType)x != 0; }; \
inline bool has(T x, T y) { return ((enumBasicType)x & (enumBasicType)y) != 0; }; \
inline enumBasicType fromFlags(T x) { return (enumBasicType)x; };

template <typename T> inline T toFlags(int x) { return (T)x; };

typedef void* HImage;
typedef void* HTheme;
typedef void* HFont;
typedef void* HThemeWidgetElement;
typedef void* HNativeWindow;
typedef void* HDockNode;
typedef void* HMouseCursor;
typedef void* HTexture;
typedef void* HContext;
typedef void* HFile;
typedef void* HFontFace;

typedef u32 Rgba32;
typedef u32 TabIndex;
typedef u32 GlyphCode;
typedef u64 DockNodeId;
typedef u64 WidgetId;
typedef std::vector<GlyphCode> Utf32String;
typedef void (*RenderCallback)(HNativeWindow wnd);

/// Horizontal align type, for text and images
enum class HAlignType
{
	Left,
	Right,
	Center
};

/// Vertical align type, for text and images
enum class VAlignType
{
	Top,
	Bottom,
	Center
};

enum class PaddingType
{
	Layout,
	ScrollView,
	Widget,

	Count
};

/// Current supported widget types, used form themes
enum class WidgetType
{
	None,
	Custom,
	Window,
	Layout,
	Tooltip,
	Button,
	ButtonGroup,
	ImageButton,
	TextInput,
	MultilineTextInput,
	Slider,
	Progress,
	Image,
	Check,
	Radio,
	Label,
	Expandable,
	TreeNode,
	Popup,
	Dropdown,
	List,
	Selectable,
	Line,
	Space,
	ScrollView,
	MenuBar,
	Menu,
	TabGroup,
	Tab,
	Viewport,
	MsgBox,
	Box,
	ComboSlider,
	CircularSlider,
	ColorPicker,
	Table,
	Link,

	Count
};

/// Current supported widget element types, used for themes
enum class WidgetElementId
{
	None = 0,
	Custom,
	WindowBody,
	ButtonBody,
	ButtonGroupLeftBody,
	ButtonGroupMiddleBody,
	ButtonGroupRightBody,
	ImageButtonBody,
	CheckBody,
	CheckMark,
	RadioBody,
	RadioMark,
	LineBody,
	LabelBody,
	ExpandableBody,
	ExpandableCollapsedArrow,
	ExpandableExpandedArrow,
	TreeNodeBody,
	TreeNodeCollapsedArrow,
	TreeNodeExpandedArrow,
	TextInputBody,
	TextInputCaret,
	TextInputSelection,
	TextInputSelectedText,
	TextInputDefaultText,
	TextInputFilterClearImage,
	MultilineTextInputBody,
	MultilineTextInputLineNumbers,
	MultilineTextInputCurrentLineHighlight,
	MultilineTextInputWordWrap,
	SliderBody,
	SliderBodyFilled,
	SliderKnob,
	ProgressBack,
	ProgressFill,
	TooltipBody,
	PopupBody,
	PopupBehind,
	DropdownBody,
	DropdownArrowBox,
	DropdownArrow,
	DropdownListBody,
	ScrollViewBody,
	ScrollViewScrollBarV,
	ScrollViewScrollThumbV,
	ScrollViewScrollBarH,
	ScrollViewScrollThumbH,
	TabGroupBody,
	TabBodyActive,
	TabBodyInactive,
	WindowHorizontalSplitter,
	WindowVerticalSplitter,
	WindowDockGuideAsTab,
	WindowDockGuideVerticalSplit,
	WindowDockGuideHorizontalSplit,
	MenuBarBody,
	MenuBarItem,
	MenuBody,
	MenuItemSeparator,
	MenuItemBody,
	MenuItemShortcut,
	MenuItemCheckMark,
	MenuItemNoCheckMark,
	SubMenuItemArrow,
	MessageBoxImageError,
	MessageBoxImageInfo,
	MessageBoxImageQuestion,
	MessageBoxImageWarning,
	SelectableBody,
	BoxBody,
	ComboSliderBody,
	ComboSliderLeftArrow,
	ComboSliderRightArrow,
	ComboSliderRangeBar,
	ComboSliderVerticalLine,
	CircularSliderBody,
	CircularSliderMark,
	CircularSliderValueDot,
	ColorPickerCheckers,
	ColorPickerBody,
	TableBody,
	TableHeaderBody,
	LinkBody,

	Count
};

/// The state of a widget
enum class WidgetStateType
{
	Normal,
	Focused,
	Pressed,
	Hovered,
	Disabled,
	Unknown,

	Count
};

/// Mouse button type
enum class MouseButton
{
	Left = 0,
	Middle,
	Right,
	AuxButton1,
	AuxButton2,
	AuxButton3,
	AuxButton4,
	AuxButton5,
	None,

	Count
};

/// OS window flags
enum class NativeWindowFlags : u32
{
	NoInput = HUI_BIT(0),
	NoDecoration = HUI_BIT(1),
	Resizable = HUI_BIT(2)
};
HUI_ENUM_AS_FLAGS(NativeWindowFlags);

enum class NativeWindowState
{
	Normal = 0,
	Minimized,
	Maximized,
	Hidden
};

enum class FileSeekMode
{
	Set = 0,
	Current,
	End
};

/// Window flags
enum class WindowFlags : u32
{
	None = HUI_BIT(0),
	Transparent = HUI_BIT(1),
	CanClose = HUI_BIT(2),
	CanMove = HUI_BIT(3),
	CanResize = HUI_BIT(4),
	CanMinimize = HUI_BIT(5),
	Disabled = HUI_BIT(6)
};
HUI_ENUM_AS_FLAGS(WindowFlags);

enum class DrawCmdLayerType : u32
{
	Normal = 0,
	Foreground,
	Overlay,
	Count
};

/// Image fit mode, used in the image widget
enum class ImageFitType
{
	None,
	KeepAspect,
	Stretch
};

/// Text input modes for the textInput widget
enum class TextInputFlags : u32
{
	None = HUI_BIT(0),
	NumericOnly = HUI_BIT(1),
	HexOnly = HUI_BIT(2),
	Custom = HUI_BIT(3),
	AutoSelectAll = HUI_BIT(4)
};
HUI_ENUM_AS_FLAGS(TextInputFlags);

enum class MultilineTextInputFlags : u32
{
	None = HUI_BIT(0),
	SpacesOnTab = HUI_BIT(1),
	LineNumbers = HUI_BIT(2),
	AutoSelectAll = HUI_BIT(3),
	HighlightCurrentLine = HUI_BIT(4),
	WordWrap = HUI_BIT(5)
};
HUI_ENUM_AS_FLAGS(MultilineTextInputFlags);

/// List selection mode
enum class ListSelectionMode
{
	Single,
	Multiple
};

/// Various flags for the selectable widget
enum class SelectableFlags : u32
{
	Normal = HUI_BIT(0),
	Checkable = HUI_BIT(1),
	Checked = HUI_BIT(2),
	Selected = HUI_BIT(3)
};
HUI_ENUM_AS_FLAGS(SelectableFlags);

/// Various flags for the tree node widget
enum class TreeNodeFlags : u32
{
	Normal = HUI_BIT(0),
	ToggleOnSelect = HUI_BIT(1)
};
HUI_ENUM_AS_FLAGS(TreeNodeFlags);

enum class TableFlags : u32
{
	None = 0,
	// Layout / sizing
	FixedFit = HUI_BIT(0),
	Stretch = HUI_BIT(1),
	FixedSize = HUI_BIT(2), // New flag: Table stays at column width sum, doesn't expand to layout
	Borders = HUI_BIT(3),
	BordersOuter = HUI_BIT(4),
	BordersInner = HUI_BIT(5),
	AltRowBg = HUI_BIT(6),
	ScrollX = HUI_BIT(7),
	Resizable = HUI_BIT(8),
	Reorderable = HUI_BIT(9),
	BordersV = HUI_BIT(10),
	BordersH = HUI_BIT(11),
};
HUI_ENUM_AS_FLAGS(TableFlags);

enum class ScrollViewFlags : u32
{
	None = 0,
	NoBorder = HUI_BIT(0),
	NoHorizontalScroll = HUI_BIT(1),
	NoPadding = HUI_BIT(2),
};
HUI_ENUM_AS_FLAGS(ScrollViewFlags);

enum class ScrollToItemSnapMode
{
	Minimal,
	AlignStart,
	AlignCenter,
	AlignEnd
};

enum class TableColumnFlags : u32
{
	None = 0,
	Fixed = HUI_BIT(0), // Cannot be resized
	FixedResize = HUI_BIT(1), // Can be resized, but maintains fixed pixel width when other columns resize
	Stretch = HUI_BIT(2), // Resizes proportionally to fill available space
};
HUI_ENUM_AS_FLAGS(TableColumnFlags);

/// When pushTint is called, specifies what element is color tinted
enum class TintColorType
{
	Body,
	Text,
	All,

	Count
};

/// When pushTint is called, specifies how is the tint color used on the widget elements
enum class TintColorOpType
{
	None,
	Multiply,
	Add,
	Subtract,
	Replace,
	Count
};

/// Key press codes
enum class KeyCode
{
	None,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,

	Num1,
	Num2,
	Num3,
	Num4,
	Num5,
	Num6,
	Num7,
	Num8,
	Num9,
	Num0,

	A,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,

	NumLock,
	Scroll,
	NumPad1,
	NumPad2,
	NumPad3,
	NumPad4,
	NumPad5,
	NumPad6,
	NumPad7,
	NumPad8,
	NumPad9,
	NumPad0,
	Multiply,
	Add,
	Subtract,
	Minus,
	Insert,
	Divide,
	Decimal,
	Home,
	End,
	PgUp,
	PgDown,
	ArrowUp,
	ArrowDown,
	ArrowLeft,
	ArrowRight,
	LControl,
	RControl,
	LShift,
	RShift,
	LAlt,
	RAlt,
	Control,
	Shift,
	Alt,
	Tab,
	Space,
	Enter,
	PrintScr,
	Esc,
	CapsLock,
	LButton,
	MButton,
	RButton,
	Pause,
	Backspace,
	LWin,
	RWin,
	Apps,
	Delete,
	Apostrophe,
	Backslash,
	Period,
	Comma,
	Equals,
	LBracket,
	RBracket,
	Semicolon,
	Slash,
	Grave,

	Count
};

enum class KeyModifiers : u32
{
	None = 0,
	Shift = HUI_BIT(0),
	Control = HUI_BIT(1),
	Alt = HUI_BIT(2),
	CapsLock = HUI_BIT(3)
};
HUI_ENUM_AS_FLAGS(KeyModifiers);

enum class MouseCursorType
{
	Arrow,
	IBeam,
	Wait,
	CrossHair,
	ArrowWait,
	SizeNWSE,
	SizeNESW,
	SizeWE,
	SizeNS,
	SizeAll,
	No,
	HandPointing,
	Custom,

	Count
};

/// Slider drag direction modes
enum class SliderDragDirection
{
	Any,
	HorizontalOnly,
	VerticalOnly
};

/// Docking modes for the windows
enum class DockType
{
	None,
	Left, /// will dock window to left
	Right, /// will dock window to right
	Top, /// will dock window to top
	Bottom, /// will dock window to bottom
	AsTab, /// will dock window as full window in the window tabs bar
	Floating /// will undock window to a floating native window
};

enum class DockNodeSplitType
{
	Top,
	Bottom,
	Left,
	Right
};

enum class DockingGuidesStyle
{
	/// Draw the docking preview as actual native windows that shape to the sides of dock nodes, wont draw the docking guides
	/// Works on Windows since we can make the dragged window pass-through events to windows below, doesnt work on Linux X11 or Wayland
	/// This mode is smoother visually, but only works on Windows
	NativeWindows,
	/// Draw the docking guides and preview inside the native windows, consistent across platforms, might be a bit flickery due to the mechanism of window dragging
	/// and updating the contents of the dragged window to match the dock indicators
	InsideNativeWindows,
	/// Only change the cursor to a special drag window cursor
	/// docking guides and preview are drawn inside the native window
	MouseCursorOnly,
	/// Automatically will choose a style based on the platform OS' capabilities
	Auto
};

/// Common message box images
enum class MessageBoxImage
{
	Error,
	Info,
	Question,
	Warning,
	Custom,

	Count
};

/// Message box flags, used for configure the messagebox and to get results
enum class MessageBoxButtons : u32
{
	None = 0,
	Ok = HUI_BIT(0),
	Cancel = HUI_BIT(1),
	Yes = HUI_BIT(2),
	No = HUI_BIT(3),
	Retry = HUI_BIT(4),
	Abort = HUI_BIT(5),
	ClosedByEscape = HUI_BIT(6), /// escape key closed the message box
	OkCancel = (u32)Ok | (u32)Cancel,
	YesNo = (u32)Yes | (u32)No,
	YesNoCancel = (u32)YesNo | (u32)Cancel
};
HUI_ENUM_AS_FLAGS(MessageBoxButtons);

enum class ContextMenuFlags
{
	None = 0
};
HUI_ENUM_AS_FLAGS(ContextMenuFlags);

enum class PopupFlags : u32
{
	None = 0,
	FadeBackground = HUI_BIT(1), /// fade the contents behind the popup when shown
	Centered = HUI_BIT(2), /// center the popup to the native window
	BelowLastWidget = HUI_BIT(3), /// position the popup below last widget
	RightSideLastWidget = HUI_BIT(4), /// position the popup on right side of the last widget
	CustomPosition = HUI_BIT(5), /// use custom popup position
	SameLayer = HUI_BIT(6), /// internal: don't increment layer index
	TopMost = HUI_BIT(7), /// set to have this popup top most
	IsMenu = HUI_BIT(8) /// internal, when this popup is a menu
};
HUI_ENUM_AS_FLAGS(PopupFlags);

enum class ColorPickerFlags : u32
{
	NoAlpha = HUI_BIT(0),
	Hdr = HUI_BIT(1),
	Float = HUI_BIT(2),
	PopupApplyButtons = HUI_BIT(3), /// show OK/Cancel buttons in color picker popup; changes only apply on OK
	ShowPalette = HUI_BIT(4) /// show default palette and optional custom color swatches
};
HUI_ENUM_AS_FLAGS(ColorPickerFlags);

enum class CircularSliderFlags : u32
{
	Normal = 0,
	ShowValueInCenter = HUI_BIT(0)
};
HUI_ENUM_AS_FLAGS(CircularSliderFlags);

/// A 2D point
struct Point
{
	Point()
		: x(0.0f)
		, y(0.0f)
	{}

	Point(const Point& other)
	{
		x = other.x;
		y = other.y;
	}

	Point(f32 newX, f32 newY)
	{
		x = newX;
		y = newY;
	}

	Point(f32 value)
	{
		x = value;
		y = value;
	}

	inline f32 dot(const Point& other) const
	{
		return x * other.x + y * other.y;
	}

	f32 getDistance(const Point& other) const
	{
		f32 xx = x - other.x, yy = y - other.y;

		xx = xx * xx + yy * yy;

		if (xx <= 0.0f)
		{
			return 0.0f;
		}

		return (f32)sqrtf(xx);
	}

	inline void makeAbsolute()
	{
		x = fabs(x);
		y = fabs(y);
	}

	f32 getCos(const Point& other) const
	{
		f32 m = (x * x + y * y) * (other.x * other.x + other.y * other.y);

		if (m <= 0.0f)
		{
			return 0.0f;
		}

		return (f32)(x * other.x + y * other.y) / sqrtf(m);
	}

	void normalize()
	{
		f32 m = x * x + y * y;

		if (m <= 0.0f)
		{
			x = y = 0.0f;
			return;
		}

		m = sqrtf(m);
		x /= m;
		y /= m;
	}

	Point getNormalized() const
	{
		f32 m = x * x + y * y;
		Point value = *this;

		if (m <= 0.0f)
		{
			return Point();
		}

		m = sqrtf((f32)m);
		value.x /= m;
		value.y /= m;

		return value;
	}

	Point& normalizeTo(Point& to)
	{
		f32 m = x * x + y * y;

		if (m <= 0.0f)
		{
			return *this;
		}

		m = sqrtf(m);
		to.x = x / m;
		to.y = y / m;

		return *this;
	}

	Point getNegated() const
	{
		Point value = *this;

		value.x = -x;
		value.y = -y;

		return value;
	}

	inline void negate()
	{
		x = -x;
		y = -y;
	}

	inline Point& negateTo(Point& to)
	{
		to.x = -x;
		to.y = -y;

		return *this;
	}

	bool isOnLine(const Point& lineA, const Point& lineB, f32 tolerance = 0.0001) const
	{
		f32 u1, u2;

		u1 = (lineB.x - lineA.x);
		u2 = (lineB.y - lineA.y);

		if (u1 == 0.0f)
			u1 = 1.0f;

		if (u2 == 0.0f)
			u2 = 1.0f;

		u1 = (x - lineA.x) / u1;
		u2 = (y - lineA.y) / u2;

		return (fabsf(u1 - u2) <= tolerance);
	}

	inline bool isAlmosEqual(const Point& other, f32 tolerance = 0.001f) const
	{
		return fabsf(x - other.x) <= tolerance
			&& fabsf(y - other.y) <= tolerance;
	}

	inline f32 getLength() const
	{
		return sqrtf(x * x + y * y);
	}

	inline f32 getSquaredLength() const
	{
		return x * x + y * y;
	}

	void setLength(f32 length)
	{
		f32 oldLen = getLength();

		if (oldLen < 0.0001f)
		{
			oldLen = 0.0001f;
		}

		f32 l = length / oldLen;

		x *= l;
		y *= l;
	}

	inline Point& set(f32 newX, f32 newY)
	{
		x = newX;
		y = newY;

		return *this;
	}

	inline Point& clear()
	{
		x = y = 0.0f;

		return *this;
	}

	inline f32 operator [](int index) const
	{
		if (index == 0)
		{
			return x;
		}
		else if (index == 1)
		{
			return y;
		}

		return 0;
	}

	inline Point& operator += (const Point& value)
	{
		x += value.x;
		y += value.y;

		return *this;
	}

	inline Point& operator *= (const Point& value)
	{
		x *= value.x;
		y *= value.y;

		return *this;
	}

	inline Point& operator -= (const Point& value)
	{
		x -= value.x;
		y -= value.y;

		return *this;
	}

	inline Point& operator /= (const Point& value)
	{
		x /= value.x;
		y /= value.y;

		return *this;
	}

	inline Point& operator += (f32 value)
	{
		x += value;
		y += value;

		return *this;
	}

	inline Point& operator *= (f32 value)
	{
		x *= value;
		y *= value;

		return *this;
	}

	inline Point& operator -= (f32 value)
	{
		x -= value;
		y -= value;

		return *this;
	}

	inline Point& operator /= (f32 value)
	{
		x /= value;
		y /= value;

		return *this;
	}

	inline Point operator + (const Point& value) const
	{
		Point result;

		result.x = x + value.x;
		result.y = y + value.y;

		return result;
	}

	inline Point operator * (const Point& value) const
	{
		Point result;

		result.x = x * value.x;
		result.y = y * value.y;

		return result;
	}

	inline Point operator - (const Point& value) const
	{
		Point result;

		result.x = x - value.x;
		result.y = y - value.y;

		return result;
	}

	inline Point operator / (const Point& value) const
	{
		Point result;

		result.x = x / value.x;
		result.y = y / value.y;

		return result;
	}

	inline Point operator + (const f32 value) const
	{
		Point result;

		result.x = x + value;
		result.y = y + value;

		return result;
	}

	inline Point operator * (const f32 value) const
	{
		Point result;

		result.x = x * value;
		result.y = y * value;

		return result;
	}

	inline Point operator - (const f32 value) const
	{
		Point result;

		result.x = x - value;
		result.y = y - value;

		return result;
	}

	inline Point operator / (const f32 value) const
	{
		Point result;

		result.x = x / value;
		result.y = y / value;

		return result;
	}

	inline Point operator / (const size_t value) const
	{
		Point result;

		result.x = x / (f32)value;
		result.y = y / (f32)value;

		return result;
	}

	inline Point& operator = (const f32 value)
	{
		x = value;
		y = value;

		return *this;
	}

	inline Point& operator = (const i32 value)
	{
		x = (f32)value;
		y = (f32)value;

		return *this;
	}

	inline bool operator <= (const Point& other) const
	{
		return x <= other.x
			&& y <= other.y;
	}

	inline bool operator >= (const Point& other) const
	{
		return x >= other.x
			&& y >= other.y;
	}

	inline bool operator < (const Point& other) const
	{
		return x < other.x
			&& y < other.y;
	}

	inline bool operator > (const Point& other) const
	{
		return x > other.x
			&& y > other.y;
	}

	inline bool operator != (const Point& other) const
	{
		return x != other.x
			|| y != other.y;
	}

	inline Point& operator = (const Point& other)
	{
		x = other.x;
		y = other.y;

		return *this;
	}

	f32 x, y;
};

/// A 2D spline control point
struct SplineControlPoint
{
	enum class NodeType
	{
		Cusp,
		Smooth,
		Symmetrical
	};

	Point leftTangent;
	Point center;
	Point rightTangent;
	bool isLine = true;
	NodeType type = NodeType::Symmetrical;
};

/// A 2D rectangle
struct Rect
{
	f32 x = 0, y = 0, width = 0, height = 0;

	Rect()
		: x(0)
		, y(0)
		, width(0)
		, height(0)
	{}

	Rect(f32 newX, f32 newY, f32 newWidth, f32 newHeight)
	{
		x = newX;
		y = newY;
		width = newWidth;
		height = newHeight;
	}

	void set(f32 newX, f32 newY, f32 newWidth, f32 newHeight)
	{
		x = newX;
		y = newY;
		width = newWidth;
		height = newHeight;
	}

	inline bool isZero() const
	{
		return x == 0.0f && y == 0.0f && width == 0.0f && height == 0.0f;
	}

	inline f32 left() const { return x; }
	inline f32 top() const { return y; }
	inline f32 right() const { return x + width; }
	inline f32 bottom() const { return y + height; }
	inline Point topLeft() const { return Point(x, y); }
	inline Point topRight() const { return Point(x + width, y); }
	inline Point bottomLeft() const { return Point(x, y + height); }
	inline Point bottomRight() const { return Point(x + width, y + height); }
	inline Point center() const { return Point(x + width / 2, y + height / 2); }
	inline bool contains(const Point& pt) const
	{
		return pt.x >= x && pt.x < (x + width) && pt.y >= y && (pt.y < y + height);
	}

	inline bool contains(f32 X, f32 Y) const
	{
		return X >= x && X < (x + width) && Y >= y && Y < (y + height);
	}

	inline bool contains(const Rect& other) const
	{
		return x <= other.x && (other.right()) < right()
			&& y <= other.y && (other.bottom()) < bottom();
	}

	inline bool outside(const Rect& other) const
	{
		return (x > other.right() || right() < other.x)
			|| (y > other.bottom() || bottom() < other.y);
	}

	Rect clipInside(const Rect& parentRect) const
	{
		Rect newRect = *this;

		if (parentRect.x > newRect.x)
		{
			newRect.width -= parentRect.x - newRect.x;
			newRect.x = parentRect.x;
		}

		if (parentRect.y > newRect.y)
		{
			newRect.height -= parentRect.y - newRect.y;
			newRect.y = parentRect.y;
		}

		if (parentRect.right() < right())
		{
			newRect.width -= right() - parentRect.right();
		}

		if (parentRect.bottom() < bottom())
		{
			newRect.height -= bottom() - parentRect.bottom();
		}

		if (newRect.width < 0) newRect.width = 0;
		if (newRect.height < 0) newRect.height = 0;

		return newRect;
	}

	inline Rect expand(f32 amount) const
	{
		return {
			x - amount,
			y - amount,
			width + 2.0f * amount,
			height + 2.0f * amount
		};
	}

	inline Rect contract(f32 amount) const
	{
		return expand(-amount);
	}

	inline Rect contract(const Point& amount) const
	{
		return expand(amount.getNegated());
	}

	inline Rect expand(const Point& amount) const
	{
		return {
			x - amount.x,
			y - amount.y,
			width + 2.0f * amount.x,
			height + 2.0f * amount.y
		};
	}

	inline Point getSize() const
	{
		return { width, height };
	}

	inline Rect operator + (const Point& pt) const
	{
		return { x + pt.x, y + pt.y, width, height };
	}

	inline Rect& operator += (const Point& pt)
	{
		x += pt.x;
		y += pt.y;
		return *this;
	}

	inline Rect operator - (const Point& pt) const
	{
		return { x - pt.x, y - pt.y, width, height };
	}

	inline Rect& operator -= (const Point& pt)
	{
		x -= pt.x;
		y -= pt.y;
		return *this;
	}

	inline Rect& operator *= (f32 amount)
	{
		width *= amount;
		height *= amount;
		return *this;
	}

	inline bool operator != (const Rect& other) const
	{
		constexpr f32 epsilon = 0.00001f;
		return fabsf(x - other.x) > epsilon
			|| fabsf(y - other.y) > epsilon
			|| fabsf(width - other.width) > epsilon
			|| fabsf(height - other.height) > epsilon;
	}

	inline Rect operator * (f32 amount)
	{
		return Rect(x, y, width * amount, height * amount);
	}
};

struct InputEvent
{
	enum class Type
	{
		None,
		MouseMove,
		MouseDown,
		MouseUp,
		MouseWheel,
		Key,
		Text,
		WindowMoved,
		WindowResized,
		WindowGotFocus,
		WindowLostFocus,
		WindowMouseEnter,
		WindowMouseLeave,
		WindowClose,
		OsDragDrop
	};

	struct MouseData
	{
		MouseButton button = MouseButton::Left;
		u32 clickCount = 1;
		Point point;
		Point wheel;
		i32 wheelDelta = 0;
		KeyModifiers modifiers = KeyModifiers::None;
	};

	struct KeyData
	{
		bool down = false;
		KeyCode code = KeyCode::None;
		KeyModifiers modifiers = KeyModifiers::None;
	};

	struct TextData
	{
		static const u32 maxTextBufferSize = 64;
		char text[maxTextBufferSize] = { 0 };
	};

	struct OsDragDropData
	{
		enum class Type
		{
			None,
			DropFile,
			DropText,
			DropBegin,
			DropComplete
		};

		Type type = Type::None;
		u32 timestamp = 0;
		char* filename = nullptr;
		HNativeWindow window = 0;
	};

	union
	{
		MouseData mouse = {};
		KeyData key;
		TextData text;
		OsDragDropData drop;
	};

	InputEvent()
	{}

	InputEvent(const InputEvent& ev)
	{
		*this = ev;
	}

	InputEvent& operator = (const InputEvent& other)
	{
		type = other.type;
		window = other.window;
		mouse = other.mouse;
		key = other.key;
		text = other.text;
		drop = other.drop;

		return *this;
	}

	Type type = Type::None;
	HNativeWindow window = 0;
};

struct HUI_STRUCT_API Color
{
	Color() {}
	Color(u32 color)
	{
		setFromRgba(color);
	}

	Color(f32 R, f32 G, f32 B, f32 A)
		: r(R), g(G), b(B), a(A)
	{}

	static Color fromU8(u8 R, u8 G, u8 B, u8 A = 255)
	{
		return Color(R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f);
	}

	u32 getRgba() const;
	u32 getArgb() const;

	void setFromRgba(u32 value)
	{
		u8 *color = (u8*)&value;

		r = (f32)color[0] / 255.0f;
		g = (f32)color[1] / 255.0f;
		b = (f32)color[2] / 255.0f;
		a = (f32)color[3] / 255.0f;
	}

	Color operator * (f32 other) const
	{
		return { r * other, g * other, b * other, a * other };
	}

	Color operator * (const Color& other) const
	{
		return { r * other.r, g * other.g, b * other.b, a * other.a };
	}

	Color operator - (const Color& other) const
	{
		return { r - other.r, g - other.g, b - other.b, a - other.a };
	}

	Color operator + (const Color& other) const
	{
		return { r + other.r, g + other.g, b + other.b, a + other.a };
	}

	operator Rgba32() const
	{
		return getRgba();
	}

	static Color random();
	static const Color transparent;
	static const Color white;
	static const Color black;
	static const Color red;
	static const Color darkRed;
	static const Color veryDarkRed;
	static const Color green;
	static const Color darkGreen;
	static const Color veryDarkGreen;
	static const Color blue;
	static const Color darkBlue;
	static const Color veryDarkBlue;
	static const Color yellow;
	static const Color darkYellow;
	static const Color veryDarkYellow;
	static const Color magenta;
	static const Color cyan;
	static const Color darkCyan;
	static const Color veryDarkCyan;
	static const Color orange;
	static const Color darkOrange;
	static const Color darkGray;
	static const Color gray;
	static const Color lightGray;
	static const Color sky;

	f32 r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
};

/// Line drawing style
struct LineStyle
{
	LineStyle() {}
	LineStyle(const Color& newColor, f32 newWidth, bool newUseStipple = false)
		: color(newColor)
		, width(newWidth)
		, useStipple(newUseStipple)
	{}
	static const u32 stipplePatternMaxCount = 8;

	Color color = Color::white;
	f32 width = 1.0f;
	bool useStipple = false;
	f32 stipplePattern[stipplePatternMaxCount] = {5, 5}; /// first value is the dash size, second is empty space size and so on, toggle
	u32 stipplePatternCount = 2;
	f32 stipplePhase = 0.0f;
};

/// Filled primitives style
struct FillStyle
{
	FillStyle() {}
	FillStyle(const Color& newColor)
		: color(newColor)
	{}

	Color color = Color::white;
	HImage image = 0;
	Point scale;
};

struct KeywordInfo
{
	enum class Type
	{
		Keyword,
		Delimiter
	};

	const char* keyword = nullptr;
	Color color = Color::white;
	Type type = Type::Keyword;
};

struct RangeHighlight
{
	const char* beginKeyword = nullptr;
	const char* endKeyword = nullptr;
	Color color = Color::white;
	const char* escapeKeyword = nullptr;
};

/// Image data info
struct ImageData
{
	Rgba32* pixels = nullptr;
	u32 width = 0;
	u32 height = 0;
};

/// Text shadow settings for embossed/outline text effects
struct TextShadow
{
	Color color;
	f32 offsetX = 1.0f;
	f32 offsetY = 1.0f;
	bool enabled = false;
};

/// Info about a widget element
struct WidgetElementInfo
{
	/// the image from the theme, used to draw the element
	HImage image = 0;
	/// the border size used to draw 9-cell resizable element
	u32 border = 0;
	/// the color of the element
	Color color;
	/// the text color of the element
	Color textColor;
	/// the font used for this element
	HFont font = 0;
	/// the pixel width of the element (not its image)
	f32 width = 0;
	/// the pixel height of the element (not its image)
	f32 height = 0;
	/// text shadow settings for embossed/outline text effects
	TextShadow textShadow;
};

struct VirtualScrollInfo
{
	VirtualScrollInfo(u32 itemCount = 0)
	{
		totalItemCount = itemCount;
	}

	u32 totalItemCount = 0; /// total number of items in the list
	f32 itemHeight = 0.0f; /// height of each item in pixels (0 = auto)

	// Results computed by nextStep()
	u32 startIndex = 0; /// inclusive index of the first item to render in the current step
	u32 endIndex = 0;   /// inclusive index of the last item to render in the current step
	f32 scrollOffsetY = 0.0f; /// current scroll offset in pixels

	bool _started = false;         // overall finished flag (nextStep returns false after complete)
	int _step = 0;                 // 0 = not started, 1 = measured-first-item, 2 = final range issued
	f32 _measureStartY = 0.0f;     // recorded y before first item drawing (for automatic measurement)
	f32 _measuredItemHeight = 0.0f;// measured item height from first item (if auto)

	bool nextStep();
};

struct DisplayInfo
{
	std::string name;
	u32 index = 0;
	Rect bounds;
	Rect usableBounds;
	f32 scale = 1.0f;
	//TODO: these worked in SDL2, SDL3 not, use scale
	//f32 diagonalDpi = 0, horizontalDpi = 0, verticalDpi = 0;
};

/// A vertex struct for rendering UI
struct Vertex
{
	Point position;
	Point uv;
	u32 color = 0xFFFFFFFF;
};

/// A render batch is a single drawcall, which renders the whole UI or part of it.
/// More render batches are generated when the various parts of the UI cannot be rendered together,
/// for example when a different texture is used or different render states
struct RenderBatch
{
	enum class PrimitiveType
	{
		TriangleList,
		TriangleStrip,
		TriangleFan
	};

	PrimitiveType primitiveType = PrimitiveType::TriangleList;
	HTexture texture = nullptr; /// which texture to use for rendering
	u32 startVertexIndex = 0; /// where to start rendering
	u32 vertexCount = 0; /// how many vertices to use for rendering the primitives
};

struct PackedRect
{
	u64 id = 0; // used to identify the rect, because the rect pack might reorder them in the rect array
	Rect rect;
	bool packedOk = false;
};

struct FontGlyph
{
	HImage image = 0; // will be created by atlas
	GlyphCode code = 0;
	f32 bearingX = 0.0f;
	f32 bearingY = 0.0f;
	f32 advanceX = 0.0f;
	f32 advanceY = 0.0f;
	i32 bitmapLeft = 0;
	i32 bitmapTop = 0;
	u32 pixelWidth = 0;
	u32 pixelHeight = 0;
	i32 pixelX = 0;
	i32 pixelY = 0;
	Rgba32* rgbaBuffer = nullptr;
};

struct FontMetrics
{
	f32 height = 0;
	f32 ascender = 0;
	f32 descender = 0;
	f32 underlinePosition = 0;
	f32 underlineThickness = 0;
};

struct FontTextSize
{
	f32 width = 0;
	f32 height = 0;
	f32 maxBearingY = 0;
	f32 maxGlyphHeight = 0;
	u32 maxLength = 0; // valid with maxWidth argument of computeTextSize is valid (!= -1)
};

struct FontInfo
{
	HFontFace fontFace = 0;
	FontMetrics metrics;
};

struct WindowsDockingState
{
	struct WindowInfo
	{
		std::string id, title;
	};

	struct DockNodeInfo
	{
		DockNodeId nodeId = 0; // unique id of the node
		DockNodeId parentNodeIndex = 0; // index of the parent node in the nodes array, root node has parentNodeIndex = 0
		DockType dockType = DockType::None; // how the window is docked in its parent node
		Rect rect;
		size_t selectedTabIndex = 0;
		std::vector<WindowInfo> windows; // the windows docked in this node
	};

	std::vector<DockNodeInfo> dockNodes; // all the dock nodes in the docking layout
};

struct Services
{
	// Input
	void (*startTextInput)(HNativeWindow window, const Rect& imeRect) = nullptr;
	void (*stopTextInput)() = nullptr;
	bool (*clipboardSetText)(const char* text) = nullptr;
	bool (*clipboardGetText)(char* outText, u32 maxTextSize) = nullptr;
	void (*processWindowEvents)(u32 timeoutMs) = nullptr;
	void (*setCurrentWindow)(HNativeWindow window) = nullptr;
	HNativeWindow (*getCurrentWindow)() = nullptr;
	HNativeWindow (*getFocusedWindow)() = nullptr;
	HNativeWindow (*getHoveredWindow)() = nullptr;
	HNativeWindow (*createWindow)(const char* title, NativeWindowFlags flags, NativeWindowState state, const Rect& rect) = nullptr;
	void (*setWindowTitle)(HNativeWindow window, const char* title) = nullptr;
	u32 (*getWindowDisplayIndex)(HNativeWindow window) = nullptr;
	u32(*getDisplayCount)() = nullptr;
	DisplayInfo(*getDisplayInfo)(u32 displayIndex) = nullptr;
	void (*setWindowSize)(HNativeWindow window, const Point& size) = nullptr;
	Point(*getWindowSize)(HNativeWindow window) = nullptr;
	void (*setWindowPosition)(HNativeWindow window, const Point& pos) = nullptr;
	Point(*getWindowPosition)(HNativeWindow window) = nullptr;
	NativeWindowState(*getWindowState)(HNativeWindow window) = nullptr;
	void (*presentWindow)(HNativeWindow window) = nullptr;
	void (*destroyWindow)(HNativeWindow window) = nullptr;
	void (*showWindow)(HNativeWindow window) = nullptr;
	void (*hideWindow)(HNativeWindow window) = nullptr;
	void (*raiseWindow)(HNativeWindow window) = nullptr;
	void (*maximizeWindow)(HNativeWindow window) = nullptr;
	void (*minimizeWindow)(HNativeWindow window) = nullptr;
	void (*setCapture)(HNativeWindow window) = nullptr;
	void (*releaseCapture)() = nullptr;
	Point (*getAbsoluteMousePosition)() = nullptr;
	void (*setAbsoluteMousePosition)(const Point& pos) = nullptr;
	bool (*isMouseButtonDownNow)(MouseButton button) = nullptr;
	void (*setCursor)(MouseCursorType type) = nullptr;
	HMouseCursor (*createCustomCursor)(Rgba32* pixels, u32 width, u32 height, u32 hotX, u32 hotY) = nullptr;
	void (*deleteCustomCursor)(HMouseCursor cursor) = nullptr;
	void (*setCustomCursor)(HMouseCursor cursor) = nullptr;
	void (*hideMouseCursor)() = nullptr;
	void (*showMouseCursor)() = nullptr;

	// Graphics
	const char* (*getGfxApiName)() = nullptr;
	void (*setViewport)(const Point& windowSize, const Rect& viewport) = nullptr;
	void (*clearBackbuffer)(const Color& color) = nullptr;
	void (*draw)(Vertex* vertices, u32 vertexCount, struct RenderBatch* batches, u32 count) = nullptr;

	// Rect packing
	bool (*packRects)(PackedRect* rects, size_t rectCount, u32 atlasWidth, u32 atlasHeight) = nullptr;

	// Fonts
	bool (*loadFont)(const char* path, u32 faceSize, FontInfo& fontInfo) = nullptr;
	bool (*loadFontFromMemory)(const void* data, size_t size, u32 faceSize, FontInfo& fontInfo) = nullptr;
	void (*freeFont)(HFontFace fontFace) = nullptr;
	f32 (*getFontKerning)(HFontFace fontFace, GlyphCode leftGlyphCode, GlyphCode rightGlyphCode) = nullptr;
	bool (*rasterizeFontGlyph)(HFontFace fontFace, GlyphCode glyphCode, FontGlyph& outGlyph) = nullptr;

	// Text encoding
	bool (*utf8To32)(const char* utf8Str, Utf32String& outUtf32Str) = nullptr;
	bool (*utf32To8)(const Utf32String& utf32Str, char** outUtf8Str) = nullptr;
	bool (*utf32To8NoAlloc)(const u32* utf32Str, size_t utf32StrSize, const char* outUtf8Str, size_t maxOutUtf8StrSize) = nullptr;
	size_t (*utf8Length)(const char* utf8Str) = nullptr;

	// File I/O
	HFile (*open)(const char* path, const char* mode) = nullptr;
	size_t (*read)(HFile file, void* outData, size_t bytesToRead) = nullptr;
	size_t (*write)(HFile file, void* data, size_t bytesToWrite) = nullptr;
	void (*close)(HFile file) = nullptr;
	bool (*seek)(HFile file, FileSeekMode mode, size_t pos) = nullptr;
	size_t (*tell)(HFile file) = nullptr;

	bool allInputFunctionsSet() const
	{
		return
			startTextInput != nullptr &&
			stopTextInput != nullptr &&
			clipboardSetText != nullptr &&
			clipboardGetText != nullptr &&
			processWindowEvents != nullptr &&
			setCurrentWindow != nullptr &&
			getCurrentWindow != nullptr &&
			getFocusedWindow != nullptr &&
			getHoveredWindow != nullptr &&
			createWindow != nullptr &&
			setWindowTitle != nullptr &&
			getWindowDisplayIndex != nullptr &&
			getDisplayCount != nullptr &&
			getDisplayInfo != nullptr &&
			setWindowSize != nullptr &&
			getWindowSize != nullptr &&
			setWindowPosition != nullptr &&
			getWindowPosition != nullptr &&
			getWindowState != nullptr &&
			presentWindow != nullptr &&
			destroyWindow != nullptr &&
			showWindow != nullptr &&
			hideWindow != nullptr &&
			raiseWindow != nullptr &&
			maximizeWindow != nullptr &&
			minimizeWindow != nullptr &&
			setCapture != nullptr &&
			releaseCapture != nullptr &&
			getAbsoluteMousePosition != nullptr &&
			setAbsoluteMousePosition != nullptr &&
			isMouseButtonDownNow != nullptr &&
			setCursor != nullptr &&
			createCustomCursor != nullptr &&
			deleteCustomCursor != nullptr &&
			setCustomCursor != nullptr &&
			hideMouseCursor != nullptr &&
			showMouseCursor != nullptr;
	}

	bool allGfxFunctionsSet() const
	{
		return getGfxApiName != nullptr &&
			setViewport != nullptr &&
			clearBackbuffer != nullptr &&
			draw != nullptr;
	}

	bool allFontFunctionsSet() const
	{
		return
			loadFont != nullptr &&
			freeFont != nullptr &&
			getFontKerning != nullptr &&
			rasterizeFontGlyph != nullptr;
	}

	bool allTextEncodingFunctionsSet() const
	{
		return
			utf8To32 != nullptr &&
			utf32To8NoAlloc != nullptr &&
			utf8Length != nullptr;
	}

	bool allFileIoFunctionsSet() const
	{
		return
			open != nullptr &&
			read != nullptr &&
			write != nullptr &&
			close != nullptr &&
			seek != nullptr &&
			tell != nullptr;
	}

	bool allFunctionsSet() const
	{
		return allInputFunctionsSet() && allGfxFunctionsSet() && allFontFunctionsSet() && allTextEncodingFunctionsSet() && allFileIoFunctionsSet();
	}
};

/// Various HorusUI per-context global settings
struct Settings
{
	Services services;
	f32 textCaretBlinkSpeed = 2.0f;
	bool textCaretBlinkEnable = true;
	f32 textScrollStepAmount = 30; /// scroll pixel amount when moving inside text input
	u32 defaultAtlasSize = 4096; /// default atlas textures size in pixels
	Point defaultLayoutPadding = {10, 10};
	Point defaultScrollViewPadding = { 10, 10 };
	Point defaultWidgetPadding = { 0, 0 };
	f32 defaultWidgetWidth = 150;
	u32 textBufferMaxSize = 1024 * 1024 * 5;/// 5MB of text on screen at once its more than enough for now
	u32 pointBufferMaxSize = 500000; /// more than enough for a full screen of lines, around 5MB
	SliderDragDirection sliderDragDirection = SliderDragDirection::Any; /// allows to change slider value from any direction drag, vertical or horizontal
	bool sliderInvertVerticalDragAmount = false; /// if true and vertical sliding allowed, it will invert the drag amount
	f32 dragStartDistance = 3; /// the max distance after which a dragging operation starts to occur when mouse down and moved, in pixels
	f32 whiteImageUvBorder = 0.001f; /// this value is subtracted from the white image used to draw lines, to avoid black border artifacts
	f32 sameLineHeight = 20.0f; /// the height of a line when sameLine() is used to position widgets on a single row/line. Used to center various widget heights vertically. This must be non-zero, otherwise the widgets will align wrongly.
	f32 minScrollViewHandleSize = 20.0f; /// the minimum allowed scroll handle size (height)
	f32 deltaTime = 0; /// you need to set this if you want to use the built in animations, otherwise they will not animate because they rely on this value to compute animation progress. This is the time elapsed between the current frame and the previous frame, in seconds
	bool scaleScrollViewHeight = false;
	bool scaleLayouts = true;
	DockingGuidesStyle dockingStyle = DockingGuidesStyle::Auto; /// use DockingGuidesStyle::InsideNativeWindows for Linux
	//TODO: this could be per native window
	bool dockAllowUndockingToNewNativeWindow = true; /// allow view tabs to be undocked as native OS windows, outside of the main window, else windows will only be allowed to dock in their owner OS windows
	f32 dockNodeSpacing = 3;
	f32 dockNodeResizeSplitterHitSize = 6;
	f32 dockNodeMinSize = 100;
	f32 dockIndicatorBoxScale = 1.0f;
	f32 dockIndicatorBoxSpacing = 4;
	f32 dockNodeDockingSizeRatio = 0.33f; /// ratio of the new size of a docked node in regard to the node we're docking in (if dockNodeProportionalResize is true)
	f32 dockNodeRootDockingHitSize = 40;
	f32 dockNodeDockingHitSizeRatio = 0.5f; /// unit percent from the size of a window used for the docking hit box
	f32 dockTabImageTextSpacing = 4;
	f32 movePopupMaxDistanceTrigger = 5; /// distance of dragging with mouse for when to initiate popup dragging
	f32 defaultBulletTextSpacing = 5; /// space size between bullet/check/radio and the label, might get overriden by the theme settings
	f32 defaultButtonGroupLabelSideSpacing = 6; /// horizontal spacing between text and segment border in button groups, might get overriden by the theme settings
	u32 tabSize = 4; /// tab size in spaces
	bool fpsThrottleEnable = false; /// if true, the UI will throttle the FPS when there is no activity
	u32 fpsThrottleMinFps = 10; /// the minimum FPS to throttle to when idle
	u32 fpsThrottleMaxFps = 60; /// the maximum FPS to run at when there is activity
	f32 fpsThrottleGradualTime = 1.0f; /// the time it takes to reach the minimum FPS when idle, in seconds
};

//////////////////////////////////////////////////////////////////////////
// Core
//////////////////////////////////////////////////////////////////////////

/// Create a new context
/// \param settings the startup context user defined settings
/// \return the created context handle
HUI_API HContext contextCreate(const Settings& settings);

/// Set the current context
/// \param ctx the context
HUI_API void contextSet(HContext ctx);

/// \return the current context
HUI_API HContext contextGet();

/// Delete a context
/// \param ctx the context to be deleted
HUI_API void contextDestroy(HContext ctx);

/// \return the context settings reference so you can read/modify them in realtime
HUI_API Settings& contextGetSettings();

/// Update the UI context, process input events, update animations, etc. This must be called once per frame, before frameBegin()
HUI_API void contextUpdate();

/// Begin a frame which means the rendering of UI across one or many windows. This must be called first when rendering UI
HUI_API void frameBegin();

/// Ends an UI frame
HUI_API void frameEnd();

HUI_API void renderBegin();
HUI_API void renderEnd();

/// Get the duration of the last UI frame in milliseconds
HUI_API f32 frameTimeGetLastMs();

/// Get the peak (maximum) UI frame time in milliseconds since app start
HUI_API f32 frameTimeGetPeakMs();

/// Get the average UI frame time in milliseconds (rolling 60 frame window)
HUI_API f32 frameTimeGetAvgMs();

/// Add a render callback at the current UI command list position 
/// A render callback is called when the UI is rendered, used to issue custom rendering commands
HUI_API void renderCallbackAdd(RenderCallback callback);

/// \return true if there is nothing to do in the UI (like redrawing or layout computations), used to not render continuously when its not needed, for applications that do not need realtime continuous rendering
HUI_API bool hasNothingToDo();

/// This will disable rendering functions, used when only widget logic needs to be run, but no drawing, used mostly internally for layout computations
/// \param disable if true, disable the rendering functions
HUI_API void skipRenderingThisFrame(bool disable);

/// Call this when you need to repaint the UI, due to data/layout changes
HUI_API void forceRepaint();

/// If called, rendering and input will be ignored until the frameEnd and the loop will redraw again, used mostly internally when layout is computed
HUI_API void skipFrame();

/// Copy UTF8 text to the clipboard
/// \param text the null ended UTF8 text
/// \return true if text was copied to clipboard
HUI_API bool clipboardSetText(const char* text);

/// Paste UTF8 from clipboard
/// \param outText a pointer to a buffer where to store the text, provided by user
/// \param maxTextSize the available text buffer size
/// \return true if text was pasted
HUI_API bool clipboardGetText(char* outText, u32 maxTextSize);

/// \return the current input event which was popped from the event queue
HUI_API const InputEvent& inputEventGet();

/// Cancel the current event, after this function call the event will be null, so no widget/window will react
HUI_API void inputEventCancel();

/// Add an input event to the queue, usually used by input providers to push events to event queue
HUI_API void inputEventAdd(const InputEvent& event);

/// Signal that the mouse was moved, used by input providers
HUI_API void inputSetMouseMoved(bool moved);

/// \return the input event count in the event queue
HUI_API size_t inputEventGetCount();

/// \return the input event at the index
/// \param index the event index (maximum is getInputEventCount())
HUI_API InputEvent inputEventGetAtIndex(size_t index);

/// Set the current input event, usually called by input providers
/// \param event the event to be set
HUI_API void inputEventSet(const InputEvent& event);

/// Clear the input event queue, usually called by input providers
HUI_API void inputEventClearQueue();

/// Set the current mouse cursor type
/// \param type the cursor type
HUI_API void mouseCursorSetType(MouseCursorType type);

/// Create a mouse cursor from a bitmap
/// \param pixels the 32bit color bitmap, RGBA
/// \param width width of the cursor bitmap
/// \param height height of the cursor bitmap
/// \param hotSpotX the cursor pointer hot spot X coordinate, relative to the bitmap size
/// \param hotSpotY the cursor pointer hot spot Y coordinate, relative to the bitmap size
/// \return the created mouse cursor
HUI_API HMouseCursor mouseCursorCreate(Rgba32* pixels, u32 width, u32 height, u32 hotSpotX = 0, u32 hotSpotY = 0);

/// Delete a custom mouse cursor
/// \param cursor the cursor to be deleted
HUI_API void mouseCursorDestroy(HMouseCursor cursor);

/// Set the current custom mouse cursor
/// \param cursor the custom mouse cursor to be set
HUI_API void mouseCursorSet(HMouseCursor cursor);

//////////////////////////////////////////////////////////////////////////
// Windowing & docking functions
//////////////////////////////////////////////////////////////////////////

HUI_API void nativeWindowSetCurrent(HNativeWindow nativeWnd);
HUI_API DockNodeId dockNodeCreateRoot(HNativeWindow nativeWnd);
HUI_API void dockNodeDeleteChildren(DockNodeId rootNodeId);
HUI_API void dockNodeSplit(DockNodeId nodeId, DockNodeSplitType splitType, f32 firstNodeSizeUnitPercent, DockNodeId* outNodeId1, DockNodeId* outNodeId2);
HUI_API void dockNodeSetWindow(DockNodeId parentNode, const char* windowId);
HUI_API void dockNodeLayoutRecalculate();

HUI_API bool windowBegin(const char* windowId, const char* title, Rect* initialRect = nullptr, HImage img = 0);
HUI_API void windowEnd();
HUI_API void windowSetVisible(const char* windowId, bool visible);
HUI_API void windowSetNextFlags(WindowFlags flags);
HUI_API void windowSetFocus(const char* windowId);
HUI_API void windowDock(const char* windowId, const char* targetWindowId, DockType dockType);
HUI_API void windowUndock(const char* windowId, const Point& windowPos = Point());
HUI_API void windowDebugPrint();
HUI_API bool windowIsMouseOver();
HUI_API void windowSetCapture();
HUI_API void windowReleaseCapture();
/// \return the window client rect
HUI_API Rect windowGetClientRect();
/// \return the window client rect, used usually to render custom scenes
HUI_API Rect windowGetClientRectByWindowId(const char* windowId);
HUI_API void dockingStateSave(WindowsDockingState& dockingState);
HUI_API void dockingStateLoad(const WindowsDockingState& dockingState);

///////////////////////////////////////////////////////////////////////////////
// Application functions
///////////////////////////////////////////////////////////////////////////////

/// Present the contents of the backbuffer for each OS native window, called after all rendering is done
HUI_API void present();

/// Present the contents of the backbuffer for a custom OS native window, called after all rendering is done
HUI_API void presentNativeWindow(HNativeWindow nativeWnd);

/// Shut down the library
HUI_API void shutdown();

//////////////////////////////////////////////////////////////////////////
// Themes
//////////////////////////////////////////////////////////////////////////

/// Set the current theme
/// \param theme the theme to be set as current
HUI_API void themeSet(HTheme theme);

/// \return the current theme
HUI_API HTheme themeGet();

HUI_API ImageData themeGetAtlasImageData();
HUI_API void themeSetAtlasTexture(HTexture texture);
HUI_API HTexture themeGetAtlasTexture();

/// Create a new theme
/// \param atlasTextureSize the width and height of the atlas texture, where theme images are kept
/// \return the newly created theme
HUI_API HTheme themeCreate(u32 atlasTextureSize);
HUI_API HTheme createBuiltinTheme(u32 atlasTextureSize);
/// Delete a theme
/// \param theme the theme to be deleted, if this is the current theme it will be set to null
HUI_API void themeDestroy(HTheme theme);
HUI_API void themeSetUserSetting(HTheme theme, const char* name, const char* value);
HUI_API const char* themeGetUserSetting(HTheme theme, const char* name);
HUI_API HImage themeAddImage(HTheme theme, const char* id, const ImageData& imgData);
HUI_API HImage themeGetImage(HTheme theme, const char* id);
HUI_API void widgetSetStyle(WidgetType widgetType, const char* styleName);
HUI_API void widgetPushStyle(WidgetType widgetType, const char* styleName);
HUI_API void widgetPopStyle();
HUI_API void widgetSetElementStyle(WidgetElementId widgetElementId, const char* styleName);
HUI_API void widgetSetDefaultStyle(WidgetType widgetType);
HUI_API void widgetSetDefaultElementStyle(WidgetElementId widgetElementId);
HUI_API void widgetSetUserElementStyle(const char* elementName, const char* styleName);

/// Set a theme's widget element info
/// \param theme the theme of the widget element
/// \param elementId the element to be set
/// \param widgetStateType which state to be set
/// \param elementInfo the element info to be set
HUI_API void themeSetWidgetElement(
	HTheme theme,
	WidgetElementId elementId,
	WidgetStateType widgetStateType,
	const WidgetElementInfo& elementInfo,
	const char* styleName = "default");

/// Build a theme after images were added to its atlas, respectively packing the theme's image atlas
/// \param theme the theme to be built
void themeBuild(HTheme theme);

/// Set a theme's user widget element info
/// \param theme the theme of the widget element
/// \param userElementName the element name
/// \param widgetStateType which state to be set
/// \param elementInfo the element info to be set
HUI_API void themeSetUserWidgetElement(
	HTheme theme,
	const char* userElementName,
	WidgetStateType widgetStateType,
	const WidgetElementInfo& elementInfo,
	const char* styleName = "default");

/// Return current theme widget element's info
/// \param elementId the widget element id
/// \param state the element state
/// \param outInfo returned element info
HUI_API void themeGetWidgetElementInfo(WidgetElementId elementId, WidgetStateType state, WidgetElementInfo& outInfo, const char* styleName = "default");
HUI_API void themeGetUserWidgetElementInfo(const char* userElementName, WidgetStateType state, WidgetElementInfo& outInfo, const char* styleName = "default");
HUI_API void themeSetWidgetElementParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const char* paramValue);
HUI_API const char* themeGetWidgetElementParameterString(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const char* defaultValue = "");
HUI_API f32 themeGetWidgetElementParameterFloat(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, f32 defaultValue = 0.0f);
HUI_API const Color& themeGetWidgetElementParameterColor(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const Color& defaultValue = Color());
HUI_API void themeSetUserWidgetElementParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const char* paramValue);
HUI_API const char* themeGetUserWidgetElementParameterString(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const char* defaultValue = "");
HUI_API f32 themeGetUserWidgetElementParameterFloat(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, f32 defaultValue = 0.0f);
HUI_API const Color& themeGetUserWidgetElementParameterColor(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const Color& defaultValue = Color());

/// Create a new font object
/// \param theme the theme where to place the font
/// \param name the name of the font (a given name like for example: 'smallItalic')
/// \param fontFilename the TTF/OTF font filename, relative to executable
/// \param faceSize the font face size in font units
/// \return the newly created font handle
HUI_API HFont themeFontCreate(HTheme theme, const char* name, const char* fontFilename, u32 faceSize);

/// Release font reference, if font usage is zero, the font is deleted
/// \param font the font to be reference released
HUI_API void themeFontDestroy(HTheme theme, HFont font);

/// \return the font by name, from the current theme
/// \param themeFontName the name of the font as it is in the theme
HUI_API HFont themeFontGet(const char* themeFontName);

/// \return the font by name, from the specified theme
HUI_API HFont themeFontGetFromTheme(HTheme theme, const char* themeFontName);

//////////////////////////////////////////////////////////////////////////
// Layouts
//////////////////////////////////////////////////////////////////////////

/// Begin a layout area, an invisible rectangle on the current window area where widgets will be laid out
HUI_API void layoutBegin(const Rect& rect);
HUI_API void layoutEnd();
HUI_API void layoutPush();
HUI_API void layoutPop();
HUI_API void idPush(const char* id);
HUI_API void idPush(u32 id);
HUI_API void idPush(void* id);
HUI_API void idPop();

HUI_API bool tableBegin(const char* id, u32 columnCount, f32 height = -1, TableFlags flags = TableFlags::None);
HUI_API void tableEnd();
HUI_API void tableStartHeader();
HUI_API void tableColumnSetup(u32 columnIndex, f32 size, TableColumnFlags flags = TableColumnFlags::None);
HUI_API void tableRowNext();
HUI_API void tableRowSetColor(const Color& color);
HUI_API void tableCellNext();
HUI_API Rect tableCellGetRect();
HUI_API void tableCellSetColor(const Color& color);
HUI_API void tableCellPaddingPush(f32 paddingX, f32 paddingY);
HUI_API void tableCellPaddingPop();

/// Get the remaining height in the current layout from current position to bottom
HUI_API f32 layoutGetRemainingHeight();

/// Get the remaining width in the current layout from current position to right edge
HUI_API f32 layoutGetRemainingWidth();
HUI_API Point layoutGetSize();

/// Begin a scroll view area widget
/// \param height the height of the scroll area
/// \param scrollPosition the current scroll position (given by endScrollView)
/// \param virtualHeight the virtual inside scroll height, if its zero then its automatically calculated from the child widgets inside this area
HUI_API void scrollViewBegin(const char* id, f32 height, f32 scrollPosition, f32 virtualHeight, ScrollViewFlags flags);
void scrollViewBegin(const char* id, f32 size, f32 scrollPos);
void scrollViewBegin(const char* id, f32 size, f32 scrollPos, f32 virtualHeight);
void scrollViewBegin(const char* id, f32 height, Point scrollOffset, Point virtualSize, ScrollViewFlags flags);

/// Ends a scroll view area widget
/// \return the current scroll position (offset)
HUI_API Point scrollViewEnd();

/// Scroll the view to the bounds of the last submitted widget (or a specific widget if id is provided).
/// When called inside a virtual list (between virtualListContentBegin/End), `id` is interpreted as the `virtualItemIndex`.
HUI_API void scrollViewScrollToWidget(WidgetId id = 0, ScrollToItemSnapMode mode = ScrollToItemSnapMode::Minimal);

/// Begin a virtual list content area, used for many items, inside the beginScrollView/endScrollView
/// \param totalRowCount the number of rows
/// \param itemHeight the height of one item
/// \param scrollPosition the current scroll offset of the scroll view widget
HUI_API void virtualListContentBegin(u32 totalRowCount, f32 itemHeight, f32 scrollPosition);

/// Begin a virtual list content area, used for many items, inside the beginScrollView/endScrollView
/// \param info the virtual scroll information
HUI_API void virtualListContentBegin(VirtualScrollInfo& info);

/// End a virtual list content area
HUI_API void virtualListContentEnd();

/// Push the old padding and set a new one, padding is the left and right side horizontal spacing for widgets
/// \param newPadding the new horizontal padding value
HUI_API void paddingPush(PaddingType type, const Point& newPadding);
HUI_API void widgetPaddingPush(const Point& newPadding);

/// Pop the previous padding value from stack and set it as current
HUI_API void paddingPop(PaddingType type);
HUI_API void widgetPaddingPop();

/// \return the current vertical spacing value
HUI_API f32 spacingGet();

/// \return the current horizontal left and right side padding value
HUI_API const Point& paddingGet(PaddingType type);

/// Begin a same-line group where all widgets get equal width from available layout width
/// \param widgetCount the number of widgets that will be in this group
HUI_API void sameLineGroupBegin(u32 widgetCount);

/// Move to the next widget position in the same-line group
HUI_API void sameLineGroupNext();

/// End the same-line group
HUI_API void sameLineGroupEnd();

// Handy version to get widget padding
HUI_API const Point& widgetGetPadding();

/// Push the old spacing value to stack and set a new spacing value, spacing is the vertical space between widgets
/// \param newSpacing the new vertical spacing value
HUI_API void spacingPush(f32 newSpacing);

/// Pop old spacing value from stack and set it as current
HUI_API void spacingPop();

/// Set the global UI scale, this will scale all the elements from widgets to text
/// \param scale a value, use with consideration, will regenerate font atlas, slow
HUI_API void scaleSet(f32 scale);

/// \return the current global UI scale
HUI_API f32 scaleGet();

/// Push and set a new tinting color on stack, to colorize the next widget on specific parts
/// \param color the tint color
/// \param type what elements of the widget to tint
HUI_API void tintPush(const Color& color, TintColorType type = TintColorType::All, TintColorOpType opType = TintColorOpType::Multiply);

/// Pop the old tint color from stack
HUI_API void tintPop();

HUI_API Color tintApply(const Color& originalColor, TintColorType type);

/// Draw a delayed tooltip widget near the previous widget
/// \param text the label of the tooltip
/// \return true if the tooltip is visible now
HUI_API bool tooltip(const char* text);

/// Begin drawing a custom tooltip (delayed), which contains other widgets like image and labels etc.
/// \param width the tooltip width
/// \return true if the tooltip is visible now
HUI_API bool customTooltipBegin(f32 width);

/// End drawing a custom tooltip
HUI_API void customTooltipEnd();

/// Begin draw a box, which may contain other widgets
/// \param color the box tint color
/// \param widgetElementId the widget element id image to use when drawing the box
/// \param state the widget element state to draw with
/// \param customHeight a forced custom height, otherwise auto calculated from the total height the child widgets have
HUI_API void boxBegin(
	const char* id,
	const Color& tintColor,
	WidgetElementId widgetElementId = WidgetElementId::BoxBody,
	WidgetStateType state = WidgetStateType::Normal,
	f32 customHeight = 0.0f);

/// Begin draw a box, which may contain other widgets
/// \param color the box tint color
/// \param userElementName the user widget element name whose image to use when drawing the box
/// \param state the widget element state to draw with
/// \param customHeight a forced custom height, otherwise auto calculated from the total height the child widgets have
HUI_API void boxBeginUserElement(
	const char* id,
	const Color& tintColor,
	const char* userElementName,
	WidgetStateType state = WidgetStateType::Normal,
	f32 customHeight = 0.0f);

/// End the box layout
HUI_API bool boxEnd();

/// Begin drawing a modal popup widget on top of all other popups or widgets
/// \param width the width of the popup
/// \param flags the popup flags
/// \param position when custom position, this is the window coordinates of the popup
/// \param widgetElementId will use this element's theme to draw the popup body
HUI_API void popupBegin(
	const char* id,
	f32 width,
	PopupFlags flags = PopupFlags::BelowLastWidget,
	const Point& position = Point(),
	WidgetElementId widgetElementId = WidgetElementId::PopupBody);

/// End a popup widget
HUI_API void popupEnd();

/// Close the current popup, used inside begin/endPopup
HUI_API void popupClose();

/// \return true if the popup must be closed, due to user input, used inside begin/endPopup
HUI_API bool popupMustClose();

/// \return true if the user clicked outside popup's rect, used inside begin/end popup
HUI_API bool popupClickedOutside();

/// \return true if mouse is outside popup's rect, used inside begin/end popup
HUI_API bool popupMouseOutside();

/// \return true if the user pressed escape while current popup is active, used inside begin/endPopup
HUI_API bool popupPressedEscape();

/// Draw a message box popup
/// \param title the message box title
/// \param message the message
/// \param buttons the visible buttons flags in the message box
/// \param img the image of the message box
/// \param width the width of the message box
/// \param customImg the custom image, if set in the image param
/// \return the pushed button in the message box
HUI_API MessageBoxButtons messageBox(
	const char* title,
	const char* message,
	MessageBoxButtons buttons = MessageBoxButtons::Ok,
	MessageBoxImage img = MessageBoxImage::Info,
	u32 width = 400,
	HImage customImg = 0);

/// Set the next widget as disabled
HUI_API void widgetSetNextDisabled(bool disabled = true);

/// Nest the disabled state, all widgets inside will be disabled
HUI_API void widgetPushDisabled(bool disabled = true);

/// Pop the disabled nesting state
HUI_API void widgetPopDisabled();

/// Returns true if the next widget will be disabled (either by parent or next-state)
HUI_API bool widgetGetDisabled();

/// Set next widget as focused
HUI_API void widgetSetNextFocused();

/// Draw a button widget
/// \param label the button text
/// \return true if button was pressed
HUI_API bool button(const char* label);
HUI_API bool buttonGroup(const char* id, const char** labels, u32 count, u32* currentIndex);
HUI_API bool buttonGroup(const char* id, const char** labels, u32 count, u32* currentIndex, bool fullWidth);

/// Draw a button with an image on it
/// \param img the image
/// \param height the button height, if zero then it takes the image's height
/// \param down if true the button is in the pressed state
/// \return true if the button was pressed
HUI_API bool imageButton(HImage img, f32 width, f32 height, HImage disabledImg = 0, bool down = false);

/// Draw a text input widget
/// \param text the text to be edited, provided by user
/// \param maxTextSize the max size of the text buffer
/// \param flags various flags for text input
/// \param defaultText the grayed default text when there is no text value
/// \param img the image drawn in the widget
/// \return true if the text was modified
HUI_API bool textInput(const char* id, char* text, u32 maxTextSize, TextInputFlags flags = TextInputFlags::None, const char* defaultText = nullptr, HImage img = 0, bool password = false, const char* passwordChar = "*");

/// Multi-line text input widget. Enter key creates a new line instead of submitting.
HUI_API bool textInputMultiline(const char* id, char* text, u32 maxTextSize, u32 visibleLines = 10, MultilineTextInputFlags flags = MultilineTextInputFlags::None, const KeywordInfo* keywords = nullptr, u32 keywordCount = 0, const RangeHighlight* rangeHighlights = nullptr, u32 rangeHighlightCount = 0);

/// Draw an integer number slider widget
/// \param minVal the minimum value
/// \param maxVal the maximum value
/// \param value the value ref
/// \param useStep use stepping when moving slider
/// \param step if useStep is true, then this is the step size
/// \return true if value was modified
HUI_API bool sliderInt(const char* id, i32 minVal, i32 maxVal, i32& value, bool useStep = false, i32 step = 0);

/// Draw a float number slider widget
/// \param minVal the minimum value
/// \param maxVal the maximum value
/// \param value the value ref
/// \param useStep use stepping when moving slider
/// \param step if useStep is true, then this is the step size
/// \return true if value was modified
HUI_API bool sliderFloat(const char* id, f32 minVal, f32 maxVal, f32& value, bool useStep = false, f32 step = 0);

HUI_API bool comboSliderInt(i32* value, f32 stepsPerPixel = 1.0f, i32 arrowStep = 1, const char* formatStr = nullptr);
HUI_API bool comboSliderIntRanged(i32* value, i32 minVal, i32 maxVal, f32 stepsPerPixel = 1, i32 arrowStep = 1.0f, const char* formatStr = nullptr);
HUI_API bool comboSliderFloat(f32* value, f32 stepsPerPixel = 1.0f, f32 arrowStep = 1.0f, const char* formatStr = nullptr);
HUI_API bool comboSliderFloatRanged(f32* value, f32 minVal, f32 maxVal, f32 stepsPerPixel = 1.0f, f32 arrowStep = 1.0f, const char* formatStr = nullptr);
HUI_API bool circularSliderFloat(const char* label, f32* value, f32 minVal, f32 maxVal, f32 step, bool twoSide = false, f32 fineStepDivideFactor = 10.f, CircularSliderFlags flags = CircularSliderFlags::Normal);

/// Draw a image widget
/// \param image the image to draw
/// \param height the height of the image, if zero then the actual image height will be used
/// \param horizontalAlign the horizontal image align mode
/// \param verticalAlign the vertical image align mode
/// \param fit how the image is fitted in the rectangle, resize mode
/// \return true if it was clicked on
HUI_API bool image(HImage image, f32 height = 0, HAlignType horizontalAlign = HAlignType::Center, VAlignType verticalAlign = VAlignType::Center, ImageFitType fit = ImageFitType::KeepAspect);

/// Draw a texture widget
/// \param texture the texture to draw
/// \param height the height of the image, if zero then the actual image height will be used
/// \param horizontalAlign the horizontal image align mode
/// \param verticalAlign the vertical image align mode
/// \param fit how the image is fitted in the rectangle, resize mode
/// \return true if it was clicked on
HUI_API bool texture(HTexture texture, f32 textureWidth, f32 textureHeight, f32 height = 0, HAlignType horizontalAlign = HAlignType::Center, VAlignType verticalAlign = VAlignType::Center, ImageFitType fit = ImageFitType::KeepAspect);

/// Draw a progress bar widget
/// \param value the progress as a percentage
HUI_API void progress(f32 value, f32 maxValue = 0.0f, bool showText = false, bool showRealValues = true, const char* indeterminateText = nullptr);

/// Draw a check box widget
/// \param label the check's label
/// \param checked true if it has check mark on
/// \return true if it was changed, result put in checked
HUI_API bool check(const char* label, bool* checkVar);

/// Draw a radio box widget
/// \param label the radio's label
/// \param currentRadioValue location of the current value of the radio group
/// \param thisValue the value of this radio button
/// \return true if it was changed, result put in checked
HUI_API bool radio(const char* label, i32* currentRadioValue, i32 thisValue);

/// Draw a label text widget
/// \param label the label's text
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HUI_API bool label(const char* label, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a label text widget with a custom font
/// \param label the label's text
/// \param font the label's font
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HUI_API bool labelCustomFont(const char* label, HFont font, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a multiline label text widget (involves more logic than a single lined label)
/// \param label the label's text
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HUI_API bool labelMultiline(const char* label, HAlignType horizontalAlign);

/// Draw a multiline label text widget with a custom font (involves more logic than a single lined label)
/// \param label the label's text
/// \param font the label's font
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HUI_API bool labelCustomFontMultiline(const char* label, HFont font, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a label text widget with a custom color
/// \param label the label's text
/// \param color the text color
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HUI_API bool labelCustomColor(const char* label, const Color& color, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a multiline label text widget with a custom color
/// \param label the label's text
/// \param color the text color
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HUI_API bool labelCustomColorMultiline(const char* label, const Color& color, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a label text widget with a custom font and color
/// \param label the label's text
/// \param font the label's font
/// \param color the text color
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HUI_API bool labelCustom(const char* label, HFont font, const Color& color, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a multiline label text widget with a custom font and color
/// \param label the label's text
/// \param font the label's font
/// \param color the text color
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HUI_API bool labelCustomMultiline(const char* label, HFont font, const Color& color, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a expandable widget
/// \param label the text of the widget
/// \param expandedVar keeps true if its expanded
/// \return true if the state changed
HUI_API bool expandable(const char* label, bool* expandedVar = nullptr);
HUI_API bool expandableBegin(const char* label, bool* expandedVar = nullptr);
HUI_API void expandableEnd();

/// Draw a tree node widget
HUI_API bool treeNode(const char* label, bool* expandedVar = nullptr, SelectableFlags stateFlags = SelectableFlags::Normal, TreeNodeFlags treeFlags = TreeNodeFlags::Normal);
HUI_API bool treeNodeBegin(const char* label, bool* expandedVar = nullptr, SelectableFlags stateFlags = SelectableFlags::Normal, TreeNodeFlags treeFlags = TreeNodeFlags::Normal);
HUI_API void treeNodeEnd();

/// Draw a dropdown widget
/// \param selectedIndex the current selected item index
/// \param items an array of strings for the items
/// \param itemCount the number of items in the list
/// \param maxVisibleDropDownItems the maximum number of visible items in the drop down list, if ~0 then its automatic
/// \return true if it the selection changed
HUI_API bool dropdown(const char* id, i32& selectedIndex, const char** items, u32 itemCount, u32 maxVisibleDropDownItems = ~0);

/// Draw a list box widget
/// \param id unique widget id
/// \param selectedItems array of bools for selection state of each item
/// \param selectionType single or multiple selection mode
/// \param items array of item label strings
/// \param itemCount number of items
/// \param height widget height
/// \param dragDropType when non-zero enables drag-drop (items are drag sources, list accepts drops of same type)
/// \return true if selection changed
HUI_API bool list(const char* id, bool* selectedItems, ListSelectionMode selectionType, const char** items, u32 itemCount, f32 height = 200.0f, u32 dragDropUserType = 0);

/// Draw a selectable label
/// \param label the selectable's text
/// \param stateFlags the state of the selectable widget
/// \return true if it is selected
HUI_API bool selectable(const char* label, SelectableFlags stateFlags = SelectableFlags::Normal);

/// Draw a selectable label with custom font
/// \param label the selectable's text
/// \param font the label's text font
/// \param stateFlags the state of the selectable widget
/// \return true if it is selected
HUI_API bool selectableCustomFont(const char* label, HFont font, SelectableFlags stateFlags = SelectableFlags::Normal);

/// Draw a hyperlink text widget
/// \param label the link text
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if the link was clicked
HUI_API bool link(const char* label, HAlignType horizontalAlign = HAlignType::Left);

//////////////////////////////////////////////////////////////////////////
// Separators
//////////////////////////////////////////////////////////////////////////

/// Draw a horizontal line widget
HUI_API void line();

/// Leave a normal space between previous widget and next one
HUI_API void space(f32 customSpacing = 0.0f);

/// Make the next widget appear on the same line as the previous widget (Dear ImGui style)
/// Call this after a widget to position the next widget horizontally
/// After each widget, same-line mode automatically ends, so you must call sameLine() again for each subsequent widget
/// \param offsetX optional additional horizontal offset
/// \param spacing optional spacing between widgets, if 0 uses default spacing
HUI_API void sameLine(f32 offsetX = 0.0f, f32 spacing = 0.0f);

HUI_API void widgetSetNextWidth(f32 width);

/// Begin a custom user viewport area
/// \param height the height of the viewport, if zero, it will take the entire remaining container height
/// \return the rectangle in window coordinates of the actual viewport area, use this to draw your custom things in
HUI_API Rect viewportBegin(const char* id, f32 height = 0);

/// End the current user viewport
HUI_API void viewportEnd();

//////////////////////////////////////////////////////////////////////////
// Menus
//////////////////////////////////////////////////////////////////////////

/// Begin a menu bar widget
HUI_API bool menuBarBegin();

/// End the current menu bar widget
HUI_API void menuBarEnd();

/// Begin a menu widget (it will show up only when clicked)
/// \param label the menu text
/// \param flags the menu flags
/// \return true if the menu is visible, use it in a if() statement to show menu items
HUI_API bool menuBegin(const char* label, SelectableFlags flags = SelectableFlags::Normal);

/// End the current menu
HUI_API void menuEnd();

/// Begin drawing a context menu which will open on right click on the previous widget
/// \return true if the menu is opened/visible
HUI_API bool contextMenuBegin(ContextMenuFlags flags = ContextMenuFlags::None);

/// End the current context menu
HUI_API void contextMenuEnd();

/// Draw a menu item widget, use inside begin/end menu (or context menu)
/// \param label the menu item text
/// \param shortcut the key shortcut text
/// \param img the menu item left side image
/// \param flags the menu item flags
/// \return true if the menu item was clicked on
HUI_API bool menuItem(const char* label, const char* shortcut = "", HImage img = 0, SelectableFlags flags = SelectableFlags::Normal);

/// Draw a menu item separator
HUI_API void menuSeparator();

//////////////////////////////////////////////////////////////////////////
// Tabs
//////////////////////////////////////////////////////////////////////////

/// Start a tab group
/// \param selectedIndex the selected tab index
HUI_API void tabGroupBegin(TabIndex selectedIndex);

/// Draw a tab widget
/// \param label the text of the tab
/// \param img the image of the tab
HUI_API void tab(const char* label, HImage img);

/// End the tab group
HUI_API TabIndex tabGroupEnd();

//////////////////////////////////////////////////////////////////////////
// Immediate state query for the last widget
//////////////////////////////////////////////////////////////////////////

/// \return true if the previous widget is hovered
HUI_API bool widgetIsHovered();

/// \return true if the previous widget is focused
HUI_API bool widgetIsFocused();

/// \return true if the previous widget is pressed down
HUI_API bool widgetIsPressed();

/// \return true if the previous widget is clicked
HUI_API bool widgetIsClicked();

/// \return true if the previous widget is visible
HUI_API bool widgetIsVisible();

/// \return true if the change for the widget's value ended, used for undo systems to add the undo action only after the drag/edit ended
HUI_API bool widgetIsChangeEnded();

/// \return the current widget id (the next widget's id)
HUI_API WidgetId widgetGetId();

HUI_API Point widgetGetPosition();
HUI_API void widgetSetPosition(const Point& position);
HUI_API void widgetPushPosition();
HUI_API void widgetPopPosition();
HUI_API Rect widgetGetRect();

/// \return the current mouse position inside current window
HUI_API Point mouseGetPosition();

/// Set the mouse position in screen coordinates
/// \param pos the position to set
HUI_API void mouseSetPosition(const Point& pos);

//////////////////////////////////////////////////////////////////////////
// Drag and drop logic support
//////////////////////////////////////////////////////////////////////////

/// \return true if there is a drag intent
HUI_API bool dragDropWantsTo();

/// set the mouse cursor to be used when dropping allowed
/// \param dropAllowedCursor the mouse cursor
HUI_API void dragDropSetMouseCursor(HMouseCursor dropAllowedCursor);

/// Begin dragging an object
/// \param dragObjectUserType the user type for the object
/// \param dragObject the user object to drag as payload
HUI_API void dragDropBegin(u32 dragObjectUserType, void* dragObject);

/// End drag and drop operation
HUI_API void dragDropEnd();

/// Allow drag drop for the next widgets
HUI_API void dragDropAllow();

/// Disallow drop for the next widgets
HUI_API void dragDropDisallow();

/// \return true if the user dropped payload on previous widget
HUI_API bool dragDropDroppedOnWidget();

/// \return the drag drop payload user object pointer
HUI_API void* dragDropGetObject();

/// \return the drag drop payload user object type
HUI_API u32 dragDropGetObjectType();

//////////////////////////////////////////////////////////////////////////
// Custom widgets
//////////////////////////////////////////////////////////////////////////

/// Begin drawing a custom widget
/// \param height the widget height
/// \return the widget rectangle in window coordinates
HUI_API Rect customWidgetBegin(const char* id, f32 height = 0.0f);

/// End custom widget drawing
HUI_API void customWidgetEnd();

HUI_API void layerIndexIncrement();
HUI_API u32 layerIndexDecrement();
HUI_API void layerDecrementWindowMaxLayerIndex();

/// Set the next widget position
HUI_API void widgetSetPosition(const Point& position);

/// \return the current widget drawing position
HUI_API Point widgetGetPosition();
HUI_API void renderDrawTextInBox(const char* text, const Rect& rect, HAlignType horizontalAlign, VAlignType verticalAlign);
HUI_API Point renderGetTextSize(const char* text);
HUI_API void renderDrawImage(HImage image, const Point& position, f32 scale);
HUI_API void renderDrawStretchedImage(HImage image, const Rect& rect);
HUI_API void renderDrawBorderedImage(HImage image, u32 border, const Rect& rect);
HUI_API void renderSetLineStyle(const LineStyle& style);
HUI_API void renderSetFillStyle(const FillStyle& style);
HUI_API void renderSetColor(const Color& color);
HUI_API void renderDrawLine(const Point& a, const Point& b);
HUI_API void renderDrawPolyLine(const Point* points, u32 pointCount, bool closed = false);
HUI_API void renderDrawCircle(const Point& center, f32 radius, u32 segments = 32);
HUI_API void renderDrawEllipse(const Point& center, f32 radiusX, f32 radiusY, u32 segments = 32);
HUI_API void renderDrawRectangle(const Rect& rc);
HUI_API void renderDrawSolidRectangle(const Rect& rc);
HUI_API void renderDrawSpline(SplineControlPoint* points, u32 count, f32 segmentSize = 15);
HUI_API void renderDrawArrow(const Point& startPoint, const Point& endPoint, f32 tipLength, f32 tipWidth, bool drawBodyLine = true);
HUI_API void renderDrawSolidTriangle(const Point& p1, const Point& p2, const Point& p3);

//////////////////////////////////////////////////////////////////////////
// Utility and complex/combined widgets
//////////////////////////////////////////////////////////////////////////

/// Draw a color picker popup widget
/// \param customColors optional array of custom swatch colors (requires ShowPalette flag)
/// \param customColorCount pointer to number of custom colors (in/out)
/// \param maxCustomColors max capacity of the customColors array
HUI_API bool colorPicker(const char* id, Color* inOutColor, ColorPickerFlags flags = (ColorPickerFlags)0, const Color* oldColor = nullptr, Color* customColors = nullptr, u32* customColorCount = nullptr, u32 maxCustomColors = 0);

/// Draw a color picker popup (a color swatch that opens a full color picker in a popup on click)
/// \param customColors optional array of custom swatch colors (requires ShowPalette flag)
/// \param customColorCount pointer to number of custom colors (in/out)
/// \param maxCustomColors max capacity of the customColors array
HUI_API bool colorPickerPopup(const char* id, Color* inOutColor, ColorPickerFlags flags = (ColorPickerFlags)0, const Color* oldColor = nullptr, Color* customColors = nullptr, u32* customColorCount = nullptr, u32 maxCustomColors = 0);

/// Draw a 3D double vector editor widget
HUI_API bool vec3Editor(const char* id, f64& x, f64& y, f64& z, f64 scrollStep = 0.03f);

/// Draw a 3D float vector editor widget
HUI_API bool vec3Editor(const char* id, f32& x, f32& y, f32& z, f32 scrollStep = 0.03f);

/// Draw a 2D double vector editor widget
HUI_API bool vec2Editor(const char* id, f64& x, f64& y, f64 scrollStep = 0.03f);

/// Draw a 2D float vector editor widget
HUI_API bool vec2Editor(const char* id, f32& x, f32& y, f32 scrollStep = 0.03f);

/// Draw an object reference editor
HUI_API bool objectRefEditor(const char* id, HImage targetImg, HImage clearImg, HImage iconImg, const char* objectTypeName, const char* valueAsString, u32 objectType, void** outObject, bool* objectValueWasModified, u32 refCount = 0, const char** refNames = nullptr, void** refValues = nullptr, f32 iconSize = 0);

//////////////////////////////////////////////////////////////////////////
// Utility functions
//////////////////////////////////////////////////////////////////////////

/// Convert an int value to string
HUI_API void stringFromI32(i32 value, char* outString, u32 outStringMaxSize, u32 fillerZeroesCount = 0);

/// Convert a float value to string
HUI_API void stringFromF32(f32 value, char* outString, u32 outStringMaxSize, i32 decimalPlaces = ~0);

HUI_API Color colorFromText(const char* colorText);
HUI_API Color colorFromHex(const char* hexText);
HUI_API u32 colorIntFromHex(const char* hexText);
HUI_API std::string colorToHex(const Color& color);
HUI_API std::string colorIntToHex(const u32 color);
HUI_API Color colorHsvToRgb(const Color& hsv);
HUI_API Color colorRgbToHsv(const Color& rgb);
HUI_API Color colorHueToRgb(f32 h, f32 alpha);

//////////////////////////////////////////////////////////////////////////
// Demo
//////////////////////////////////////////////////////////////////////////

/// Draw a demo window showcasing all widgets in all argument modes, organized in expandable sections
HUI_API void showDemo();

}
