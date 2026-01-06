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

/// \file horus.h

#ifdef HORUS_CUSTOM_CONFIG_FILE
#include HORUS_CUSTOM_CONFIG_FILE
#endif

#ifndef HORUS_NO_BASIC_TYPES
#ifndef HORUS_NO_U8
typedef uint8_t u8;
#endif

#ifndef HORUS_NO_U16
typedef uint16_t u16;
#endif

#ifndef HORUS_NO_U32
typedef uint32_t u32;
#endif

#ifndef HORUS_NO_U64
typedef uint64_t u64;
#endif

#ifndef HORUS_NO_U128
typedef struct
{
	u64 data[2];
} u128;
#endif

#ifndef HORUS_NO_I8
typedef int8_t i8;
#endif

#ifndef HORUS_NO_I16
typedef int16_t i16;
#endif

#ifndef HORUS_NO_I32
typedef int32_t i32;
#endif

#ifndef HORUS_NO_I64
typedef int64_t i64;
#endif

#ifndef HORUS_NO_I128
typedef struct
{
	i64 data[2];
} i128;
#endif

#ifndef HORUS_NO_F32
typedef float f32;
#endif

#ifndef HORUS_NO_F64
typedef double f64;
#endif

#endif

#ifdef HORUS_STATIC
	#define HORUS_API
	#define HORUS_STRUCT_API
#else
#ifdef _WINDOWS
	#ifdef HORUS_EXPORT
		#define HORUS_API extern "C++" __declspec(dllexport)
		#define HORUS_STRUCT_API __declspec(dllexport)
	#else
		#ifdef HORUS_IMPORT
			#define HORUS_API extern "C++" __declspec(dllimport)
			#define HORUS_STRUCT_API __declspec(dllimport)
		#else
			#define HORUS_API
			#define HORUS_STRUCT_API
		#endif
	#endif
#else
	#ifdef HORUS_EXPORT
		#define HORUS_API __attribute__((dllexport))
		#define HORUS_STRUCT_API __attribute__((dllexport))
	#else
		#ifdef HORUS_IMPORT
			#define HORUS_API __attribute__((dllimport))
			#define HORUS_STRUCT_API __attribute__((dllimport))
		#else
			#define HORUS_API
			#define HORUS_STRUCT_API
		#endif
	#endif
#endif
#endif

#ifndef HORUS_ASSERT
#include <assert.h>
#define HORUS_ASSERT(cond) assert(cond)
#endif

#ifndef HORUS_LOG
#define HORUS_LOG(format, ...) printf(format"\n", ##__VA_ARGS__)
#endif

namespace hui
{
#define HORUS_BIT(bit) (1<<bit)
#define HORUS_ENUM_AS_FLAGS(T)\
	HORUS_ENUM_AS_FLAGS_EX(T, u32)
#define HORUS_ENUM_AS_FLAGS_EX(T, enumBasicType) \
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

// Some shortcuts for the service providers
#define HORUS_FILE hui::getSettings().providers.file
#define HORUS_FILEDIALOGS hui::getSettings().providers.fileDialogs
#define HORUS_GFX hui::getSettings().providers.gfx
#define HORUS_INPUT hui::getSettings().providers.input
#define HORUS_UTF hui::getSettings().providers.utf
#define HORUS_IMAGE hui::getSettings().providers.image
#define HORUS_FONT hui::getSettings().providers.font
#define HORUS_RECTPACK hui::getSettings().providers.rectPack

typedef void* HImage;
typedef void* HTheme;
typedef void* HAtlas;
typedef void* HFont;
typedef void* HThemeWidgetElement;
typedef void* HNativeWindow;
typedef void* HDockNode;
typedef void* HMouseCursor;
typedef void* HGraphicsApiContext;
typedef void* HGraphicsApiTexture;
typedef void* HGraphicsApiRenderTarget;
typedef void* HGraphicsApiVertexBuffer;
typedef void* HContext;
typedef void* HFile;
typedef void* HRectPacker;
typedef void* HFontFace;

typedef u32 Rgba32;
typedef u32 TabIndex;
typedef u32 GlyphCode;
typedef std::vector<GlyphCode> Utf32String;
typedef u64 DockNodeId;
typedef u64 WidgetId;

typedef void (*RenderCallback)(HNativeWindow wnd);

const f32 ColumnFill = -1;

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
	Compound,
	Tooltip,
	Button,
	ImageButton,
	TextInput,
	Slider,
	Progress,
	Image,
	Check,
	Radio,
	Label,
	Expandable,
	Panel,
	Popup,
	Dropdown,
	List,
	Selectable,
	ResizeGrip,
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
	Toolbar,
	ToolbarButton,
	ToolbarDropdown,
	ToolbarSeparator,
	ColumnsHeader,
	ComboSlider,
	RotarySlider,
	ColorPicker,

	Count
};

/// Current supported widget element types, used for themes
enum class WidgetElementId
{
	None = 0,
	Custom,
	WindowBody,
	ButtonBody,
	ImageButtonBody,
	CheckBody,
	CheckMark,
	RadioBody,
	RadioMark,
	LineBody,
	LabelBody,
	PanelBody,
	PanelTitleBody,
	PanelCloseButton,
	PanelResizeHandle,
	ExpandableBody,
	ExpandableCollapsedArrow,
	ExpandableExpandedArrow,
	TextInputBody,
	TextInputCaret,
	TextInputSelection,
	TextInputDefaultText,
	TextInputFilterClearImage,
	SliderBody,
	SliderBodyFilled,
	SliderKnob,
	ProgressBack,
	ProgressFill,
	TooltipBody,
	PopupBody,
	PopupBehind,
	DropdownBody,
	DropdownArrow,
	ScrollViewBody,
	ScrollViewScrollBar,
	ScrollViewScrollThumb,
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
	ToolbarBody,
	ToolbarButtonBody,
	ToolbarDropdownBody,
	ToolbarSeparatorVerticalBody,
	ToolbarSeparatorHorizontalBody,
	ColumnsHeaderBody,
	ComboSliderBody,
	ComboSliderLeftArrow,
	ComboSliderRightArrow,
	ComboSliderRangeBar,
	ComboSliderVerticalLine,
	RotarySliderBody,
	RotarySliderMark,
	RotarySliderValueDot,
	ColorPickerCheckers,
	ColorPickerBody,

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
	NoInput = HORUS_BIT(0),
	NoDecoration = HORUS_BIT(1),
	Resizable = HORUS_BIT(2)
};
HORUS_ENUM_AS_FLAGS(NativeWindowFlags);

enum class NativeWindowState
{
	Normal,
	Minimized,
	Maximized,
	Hidden
};

/// Window flags
enum class WindowFlags : u32
{
	None = HORUS_BIT(0),
	Transparent = HORUS_BIT(1),
	CanClose = HORUS_BIT(2),
	CanMove = HORUS_BIT(3),
	CanResize = HORUS_BIT(4),
	CanMinimize = HORUS_BIT(5),
	Disabled = HORUS_BIT(6)
};
HORUS_ENUM_AS_FLAGS(WindowFlags);

/// Image fit mode, used in the image widget
enum class ImageFitType
{
	None,
	KeepAspect,
	Stretch
};

/// Text input modes for the textInput widget
enum class TextInputValueMode
{
	Any,
	NumericOnly,
	HexOnly,
	Custom
};

/// List selection mode
enum class ListSelectionMode
{
	Single,
	Multiple
};

/// Various flags for the selectable widget
enum class SelectableFlags : u32
{
	Normal = HORUS_BIT(0),
	Checkable = HORUS_BIT(1),
	Checked = HORUS_BIT(2),
	Disabled = HORUS_BIT(3),
	Selected = HORUS_BIT(4)
};
HORUS_ENUM_AS_FLAGS(SelectableFlags);

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
	Shift = HORUS_BIT(0),
	Control = HORUS_BIT(1),
	Alt = HORUS_BIT(2),
	CapsLock = HORUS_BIT(3)
};
HORUS_ENUM_AS_FLAGS(KeyModifiers);

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

/// Text cache pruning mode. The text cache is keeping unicode text transformed from utf8 to be faster to render each frame
/// When the UI is rendered continuously every frame the cache is pruned for non used text, based on last time access.
/// When the UI is rendered only when needed, the cache is pruned for non used text, based on frame count, if that is greater than a specified max frames, then the unicode text is discarded from cache.
enum class TextCachePruneMode
{
	Time, /// delete unused text after some time
	Frames /// delete unused text after N frames
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
	Ok = HORUS_BIT(0),
	Cancel = HORUS_BIT(1),
	Yes = HORUS_BIT(2),
	No = HORUS_BIT(3),
	Retry = HORUS_BIT(4),
	Abort = HORUS_BIT(5),
	ClosedByEscape = HORUS_BIT(6), /// escape key closed the message box
	OkCancel = (u32)Ok | (u32)Cancel,
	YesNo = (u32)Yes | (u32)No,
	YesNoCancel = (u32)YesNo | (u32)Cancel
};
HORUS_ENUM_AS_FLAGS(MessageBoxButtons);

enum class ContextMenuFlags
{
	None = 0,
	AllowLeftClickOpen = HORUS_BIT(1)
};
HORUS_ENUM_AS_FLAGS(ContextMenuFlags);

enum class PopupFlags : u32
{
	None = 0,
	FadeBackground = HORUS_BIT(1), /// fade the contents behind the popup when shown
	Centered = HORUS_BIT(2), /// center the popup to the native window
	BelowLastWidget = HORUS_BIT(3), /// position the popup below last widget
	RightSideLastWidget = HORUS_BIT(4), /// position the popup on right side of the last widget
	CustomPosition = HORUS_BIT(5), /// use custom popup position
	SameLayer = HORUS_BIT(6), /// internal: don't increment layer index
	TopMost = HORUS_BIT(7), /// set to have this popup top most
	IsMenu = HORUS_BIT(8) /// internal, when this popup is a menu
};
HORUS_ENUM_AS_FLAGS(PopupFlags);

enum class ColorPickerFlags : u32
{
	NoAlpha = HORUS_BIT(0),
	Hdr = HORUS_BIT(1),
	Float = HORUS_BIT(2)
};
HORUS_ENUM_AS_FLAGS(ColorPickerFlags);

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
		static const int maxTextBufferSize = 64;
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

struct HORUS_STRUCT_API Color
{
	Color() {}
	Color(u32 color)
	{
		setFromRgba(color);
	}

	Color(f32 R, f32 G, f32 B, f32 A)
		: r(R), g(G), b(B), a(A)
	{}

	static Color fromU8(u8 R, u8 G, u8 B, u8 A)
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

/// Used by the openMultipleFileDialog function. Warning! the pointers will be deleted on struct's instance out of scope
struct OpenMultipleFileSet
{
	char* filenameBuffer = nullptr; /// buffer used to store the filenames, created by the library
	size_t* bufferIndices = nullptr; /// array containing indices into filenameBuffer, where each filename starts
	u32 count = 0; /// the number of filenames

	~OpenMultipleFileSet();
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
	HGraphicsApiTexture texture = 0;
	Point scale;
};

/// Image data info
struct ImageData
{
	u8* pixels = nullptr;
	u32 width = 0;
	u32 height = 0;
	u32 bpp = 0;
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
};

struct ServiceProviders
{
	struct InputProvider* input = 0;
	struct GraphicsProvider* gfx = 0;
	struct ImageProvider* image = 0;
	struct FileProvider* file = 0;
	struct FileDialogsProvider* fileDialogs = 0;
	struct UtfProvider* utf = 0;
	struct FontProvider* font = 0;
	struct RectPackProvider* rectPack = 0;
	struct LogProvider* log = 0;
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

/// Various HorusUI per-context global settings
struct Settings
{
	ServiceProviders providers;
	TextCachePruneMode textCachePruneMode = TextCachePruneMode::Time; /// how to prune the unicode text cache which is not used for a while
	f32 textCachePruneMaxTimeSec = 5; /// after this time, if an Unicode text is not accessed, it's discarded from cache, textCachePruneMode must be Time
	f32 textCachePruneMaxFrames = 500; /// after this frame count, if an Unicode text is not accessed, it's discarded from cache, textCachePruneMode must be Frames
	f32 textCachePruneIntervalSec = 5; /// after each interval has passed, the pruning of unused texts is executed, will delete the texts that were not used for the last textCachePruneMaxTimeMs or textCachePruneMaxFrames, depending on the prune mode
	f32 textCaretBlinkDelay = 0.4f;
	bool textCaretBlinkEnable = true;
	f32 textScrollStepAmount = 30; /// scroll pixel amount when moving inside text input
	u32 defaultAtlasSize = 4096; /// default atlas textures size in pixels
	Point defaultLayoutPadding = {10, 10};
	Point defaultScrollViewPadding = { 10, 10 };
	Point defaultWidgetPadding = { 0, 0 };
	SliderDragDirection sliderDragDirection = SliderDragDirection::Any; /// allows to change slider value from any direction drag, vertical or horizontal
	bool sliderInvertVerticalDragAmount = false; /// if true and vertical sliding allowed, it will invert the drag amount
	f32 dragStartDistance = 3; /// the max distance after which a dragging operation starts to occur when mouse down and moved, in pixels
	f32 whiteImageUvBorder = 0.001f; /// this value is subtracted from the white image used to draw lines, to avoid black border artifacts
	f32 sameLineHeight = 20.0f; /// the height of a line when sameLine() is used to position widgets on a single row/line. Used to center various widget heights vertically. This must be non-zero, otherwise the widgets will align wrongly.
	f32 minScrollViewHandleSize = 20.0f; /// the minimum allowed scroll handle size (height)
	bool scaleScrollViewHeight = false;
	bool scaleContainers = true;
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
};

enum class FileSeekMode
{
	Current = 1,
	End = 2,
	Set = 0,
};

struct FileProvider
{
	virtual ~FileProvider() {};
	virtual HFile open(const char* path, const char* mode) = 0;
	virtual size_t read(HFile file, void* outData, size_t bytesToRead) = 0;
	virtual size_t write(HFile file, void* data, size_t bytesToWrite) = 0;
	virtual void close(HFile file) = 0;
	virtual bool seek(HFile file, FileSeekMode mode, size_t pos = 0) = 0;
	virtual size_t tell(HFile file) = 0;
};

struct FileDialogsProvider
{
	virtual ~FileDialogsProvider() {}
	/// Show an open file dialog
	virtual bool openFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize) = 0;
	/// Show an open multiple file dialog
	virtual bool openMultipleFileDialog(const char* filterList, const char* defaultPath, OpenMultipleFileSet& outPathSet) = 0;
	/// Show an save file dialog
	virtual bool saveFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize) = 0;
	/// Show a pick folder dialog
	virtual bool pickFolderDialog(const char* defaultPath, char* outPath, u32 maxOutPathSize) = 0;
};

/// The input provider is used for input and windowing services
struct InputProvider
{
	virtual ~InputProvider() {}
	/// Start text input, usually called by the library to show IME suggestions boxes
	/// \param window the window where the text started to be input
	/// \param imeRect the rectangle where to show the suggestion box
	virtual void startTextInput(HNativeWindow window, const Rect& imeRect) = 0;

	/// Called when the text input ends
	virtual void stopTextInput() = 0;

	/// Copy UTF8 text to clipboard
	/// \param text UTF8 text
	/// \return true if all ok and text was copied to clipboard
	virtual bool copyToClipboard(const char* text) = 0;

	/// Paste UTF8 text from clipboard
	/// \param outText user text buffer, already allocated
	/// \param maxTextSize the user text buffer size
	/// \return true if the paste into the buffer was successful
	virtual bool pasteFromClipboard(char* outText, u32 maxTextSize) = 0;

	/// Process the events in the queue, place events in the library's queue
	virtual void processEvents() = 0;

	/// Set the current native window, where drawing and input testing is occurring
	virtual void setCurrentWindow(HNativeWindow window) = 0;

	/// \return the current native window
	virtual HNativeWindow getCurrentWindow() = 0;

	/// \return the focused native window
	virtual HNativeWindow getFocusedWindow() = 0;

	/// \return the hovered native window
	virtual HNativeWindow getHoveredWindow() = 0;

	/// Create a new native window
	/// \param title the window title, UTF8 text
	/// \param width the window width
	/// \param height the window height
	/// \param flags the window flags
	/// \param customPosition if the positionType is custom, then this is the window's initial position
	/// \return the new window handle
	virtual HNativeWindow createWindow(const char* title, NativeWindowFlags flags, NativeWindowState state, const Rect& rect) = 0;

	/// Set window title
	/// \param window the window
	/// \param title UTF8 text for the title
	virtual void setWindowTitle(HNativeWindow window, const char* title) = 0;

	/// Get window title
	virtual std::string getWindowTitle(HNativeWindow window) = 0;

	virtual u32 getWindowDisplayIndex(HNativeWindow window) = 0;

	virtual u32 getDisplayCount() const = 0;

	virtual DisplayInfo getDisplayInfo(u32 displayIndex) = 0;

	/// Set the window client area size
	/// \param window the window
	/// \param size the width and height
	virtual void setWindowSize(HNativeWindow window, const Point& size) = 0;

	/// Get the window client area size
	/// \param window the window
	virtual Point getWindowSize(HNativeWindow window) = 0;

	/// Set the window absolute screen position
	/// \param window the window
	/// \param pos the position
	virtual void setWindowPosition(HNativeWindow window, const Point& pos) = 0;

	/// Get the window absolute screen position
	/// \param window the window
	virtual Point getWindowPosition(HNativeWindow window) = 0;

	/// Return the window current state  
	virtual NativeWindowState getWindowState(HNativeWindow window) = 0;

	/// Present the backbuffer of the specified window
	/// \param window the window to present
	virtual void presentWindow(HNativeWindow window) = 0;

	/// Destroy a native window
	/// \param window the window
	virtual void destroyWindow(HNativeWindow window) = 0;

	/// Show a native window
	/// \param window the window to show
	virtual void showWindow(HNativeWindow window) = 0;

	/// Hide a native window
	/// \param window the window to hide
	virtual void hideWindow(HNativeWindow window) = 0;

	/// Bring a native window to front of all windows, on supported OS-es
	/// \param window the window
	virtual void raiseWindow(HNativeWindow window) = 0;

	/// Maximize a native window
	/// \param window the window
	virtual void maximizeWindow(HNativeWindow window) = 0;

	/// Minimize a native window
	/// \param window the window
	virtual void minimizeWindow(HNativeWindow window) = 0;

	/// Set the input capture to a specified window
	/// \param window the window
	virtual void setCapture(HNativeWindow window) = 0;

	/// Release capture from the captured window (if any)
	virtual void releaseCapture() = 0;

	/// \return the current screen mouse position
	virtual Point getAbsoluteMousePosition() = 0;

	/// \return true if the mouse button is down right now, no matter the events
	virtual bool isMouseButtonDownNow(MouseButton button) = 0;

	/// Set the current mouse cursor type
	/// \param type the mouse cursor type
	virtual void setCursor(MouseCursorType type) = 0;

	/// Create a custom mouse cursor
	/// \param pixels the mouse cursor image as 32bit RGBA
	/// \param width mouse cursor image width
	/// \param height mouse cursor image height
	/// \param x mouse cursor x hot spot in the image
	/// \param y mouse cursor y hot spot in the image
	/// \return the new mouse cursor handle
	virtual HMouseCursor createCustomCursor(Rgba32* pixels, u32 width, u32 height, u32 hotX, u32 hotY) = 0;

	/// Delete a custom mouse cursor
	/// \param cursor the cursor handle
	virtual void deleteCustomCursor(HMouseCursor cursor) = 0;

	/// Set the current mouse cursor to a custom cursor
	/// \param cursor the custom cursor handle
	virtual void setCustomCursor(HMouseCursor cursor) = 0;

	/// Shutdown the input provider
	virtual void shutdown() = 0;
};

/// A vertex struct for rendering UI
struct Vertex
{
	Point position;
	Point uv;
	u32 color = 0xffffffff;
	u32 textureIndex = 0; /// what atlas texture array index this vertex is using
};

/// A graphics texture array
struct TextureArray
{
	virtual ~TextureArray() {}

	/// Resize the texture array, this will not preserve the current texture data
	/// \param count the new number of textures in the array
	/// \param newWidth the new width, ideally power of two
	/// \param newHeight the new height, ideally power of two
	virtual void resize(u32 count, u32 newWidth, u32 newHeight) = 0;

	/// Update the texture array data, this is the whole array of textures, no mipmaps
	virtual void updateData(Rgba32* pixels) = 0;

	/// Update a specified texture in the array
	/// \param textureIndex the 0-based texture index to be updated
	/// \param pixels the RGBA 32bit pixel buffer
	virtual void updateLayerData(u32 textureIndex, Rgba32* pixels) = 0;

	/// Update a specified texture area defined by a rectangle, in the texture array
	/// \param textureIndex the 0-based texture index to be updated
	/// \param rect the rectangle area to be updated
	/// \param pixels the RGBA 32bit pixel buffer
	virtual void updateRectData(u32 textureIndex, const Rect& rect, Rgba32* pixels) = 0;

	/// \return the graphics API handle of the texture, you may cast it to the proper handle for your graphics API
	virtual HGraphicsApiTexture getHandle() const = 0;

	/// \return the textures width
	virtual u32 getWidth() const = 0;

	/// \return the textures height
	virtual u32 getHeight() const = 0;

	/// \return the textures count
	virtual u32 getCount() const = 0;
};

/// A vertex buffer used to hold UI vertices
struct VertexBuffer
{
	virtual ~VertexBuffer() {}

	/// Resize the vertex buffer, it will not keep the old contents
	virtual void resize(u32 count) = 0;

	/// Update the vertex data on a specified range
	/// \param vertices the new vertex data slice
	/// \param startVertexIndex the start vertex index offset
	/// \param count the vertex count to update
	virtual void updateData(Vertex* vertices, u32 startVertexIndex, u32 count) = 0;

	/// \return the graphics API handle for this vertex buffer, you may cast it to the proper handle your graphics API uses
	virtual HGraphicsApiVertexBuffer getHandle() const = 0;
};

/// A render batch is a single drawcall, which renders the whole UI or part of it.
/// More render batches are generated when the various parts of the UI cannot be rendered together,
/// for example when a different texture atlas is used or different render states
struct RenderBatch
{
	enum class PrimitiveType
	{
		TriangleList,
		TriangleStrip,
		TriangleFan
	};

	PrimitiveType primitiveType = PrimitiveType::TriangleList;
	VertexBuffer* vertexBuffer = nullptr; /// which vertex buffer to use for rendering
	TextureArray* textureArray = nullptr; /// which texture array to use for rendering
	HAtlas atlas = nullptr; /// handle to the corresponding image atlas
	u32 startVertexIndex = 0; /// where to start rendering
	u32 vertexCount = 0; /// how many vertices to use for rendering the primitives
	/// The draw command callback is used when the user wants to render this batch
	typedef void(*DrawCommandCallback)(void* userdata, RenderBatch& batch);
	/// User defined command callback
	DrawCommandCallback commandCallback = nullptr;
};

/// The graphics provider, used to render UI
struct GraphicsProvider
{
	/// The supported graphics APIs
	enum class ApiType
	{
		OpenGL,
		Vulkan,
		Metal,
		Direct3D11,
		Direct3D12,
		Custom,

		Count
	};

	virtual ~GraphicsProvider() {}

	/// Initialize the graphics provider and it's API objects
	/// \return true if all ok
	virtual bool initialize() = 0;

	/// Destroy the graphics provider's API objects
	virtual void shutdown() = 0;

	/// \return the graphics API type
	virtual ApiType getApiType() const = 0;

	/// Create a new texture array object used for UI image atlas
	virtual TextureArray* createTextureArray() = 0;

	/// Create a new vertex buffer
	/// \return new vertex buffer
	virtual VertexBuffer* createVertexBuffer() = 0;

	/// Create a new render target texture
	/// \param width the texture width
	/// \param height the texture height
	virtual HGraphicsApiRenderTarget createRenderTarget(u32 width, u32 height) = 0;

	/// Delete a render target
	virtual void destroyRenderTarget(HGraphicsApiRenderTarget rt) = 0;

	/// Set the current render target
	virtual void setRenderTarget(HGraphicsApiRenderTarget rt) = 0;

	/// Set the current viewport and scissor box
	/// \param windowSize the native window's current size
	/// \param viewport the viewport with top-left corner as (0,0)
	virtual void setViewport(const Point& windowSize, const Rect& viewport) = 0;

	virtual Rect getViewport() const = 0;

	/// Clear the current backbuffer with a specified color
	virtual void clear(const Color& color) = 0;

	/// Draw the given render batch array
	virtual void draw(struct RenderBatch* batches, u32 count) = 0;
};

struct PackRect
{
	u32 id = 0; // used to identify the rect, because the rect pack might reorder them in the rect array
	Rect rect;
	bool packedOk = false;
};

struct RectPackProvider
{
	virtual HRectPacker createRectPacker() = 0;
	virtual void deleteRectPacker(HRectPacker packer) = 0;
	virtual void reset(HRectPacker packer, u32 atlasWidth, u32 atlasHeight) = 0;
	virtual bool packRects(HRectPacker packer, PackRect* rects, size_t rectCount) = 0;
};

struct FontGlyph
{
	HImage image = nullptr; // will be created by atlas
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

struct FontKerningPair
{
	GlyphCode glyphLeft = 0;
	GlyphCode glyphRight = 0;
	f32 kerning = 0.0f;
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
	u32 lastFontIndex = 0;
	u32 maxLength = 0; // valid with maxWidth argument of computeTextSize is valid (!= -1)
};

struct FontInfo
{
	HFontFace fontFace = 0;
	FontMetrics metrics;
};

struct FontProvider
{
	virtual ~FontProvider() {}
	virtual bool loadFont(const char* path, u32 faceSize, FontInfo& fontInfo) = 0;
	virtual void freeFont(HFontFace fontFace) = 0;
	virtual f32 getKerning(HFontFace fontFace, GlyphCode leftGlyphCode, GlyphCode rightGlyphCode) = 0;
	virtual bool rasterizeGlyph(HFontFace fontFace, GlyphCode glyphCode, FontGlyph& outGlyph) = 0;
};

struct ImageProvider
{
	virtual ~ImageProvider() {}
	virtual bool loadImage(const char* path, ImageData& outImage) = 0;
	virtual bool savePngImage(const char* path, const ImageData& image) = 0;
};

struct UtfProvider
{
	virtual ~UtfProvider() {}
	virtual bool utf8To32(const char* utf8Str, Utf32String& outUtf32Str) = 0;
	virtual bool utf32To16(const Utf32String& utf32Str, wchar_t** outUtf16Str, size_t& outUtf16StrLen) = 0;
	virtual bool utf32To8(const Utf32String& utf32Str, char** outUtf8Str) = 0;
	virtual bool utf16To8(const wchar_t* utf16Str, char** outUtf8Str) = 0;
	virtual bool utf32To8NoAlloc(const Utf32String& utf32Str, const char* outUtf8Str, size_t maxUtf8StrLen) = 0;
	virtual bool utf32To8NoAlloc(const u32* utf32Str, size_t utf32StrSize, const char* outUtf8Str, size_t maxOutUtf8StrSize) = 0;
	virtual size_t utf8Length(const char* utf8Str) = 0;
};

//////////////////////////////////////////////////////////////////////////
// Core
//////////////////////////////////////////////////////////////////////////

/// Create a new HorusUI context
/// \param settings context settings
/// \return the created context handle
HORUS_API HContext createContext(struct Settings& settings);

/// Set the current context
/// \param ctx the context
HORUS_API void setContext(HContext ctx);

/// \return the current context
HORUS_API HContext getContext();

/// Delete a context
/// \param ctx the context to be deleted
HORUS_API void deleteContext(HContext ctx);

/// \return the context settings reference so you can modify them in realtime
HORUS_API Settings& getSettings();

HORUS_API void initializeRenderer();

/// Set the current frame time delta. Used for tooltips and other timed things.
/// Must be called continuously in the main loop. If initializeWithSDL is used, no need to call it, the SDL input provider will update it.
/// \param dt delta time value, in seconds
HORUS_API void setFrameDeltaTime(f32 dt);

/// \return delta time in seconds
HORUS_API f32 getFrameDeltaTime();

HORUS_API void update();

/// Begin a frame which means the rendering of UI across one or many windows. This must be called first when rendering UI
HORUS_API void beginFrame();

/// Ends an UI frame
HORUS_API void endFrame();

HORUS_API void addRenderCallback(RenderCallback callback);

HORUS_API void clearBackground(const Color& color);

/// \return true if there is nothing to do in the UI (like redrawing or layout computations), used to not render continuously when its not needed, for applications that do not need realtime continuous rendering
HORUS_API bool hasNothingToDo();

/// This will disable rendering functions, used when only widget logic needs to be run, but no drawing, used mostly internally for layout computations
/// \param disable if true, disable the rendering functions
HORUS_API void setDisableRendering(bool disable);

/// Call this when you need to repaint the UI, due to data/layout changes
HORUS_API void forceRepaint();

/// If called, rendering and input will be ignored until the endFrame and the loop will redraw again, used mostly internally when layout is computed
HORUS_API void skipThisFrame();

/// Copy UTF8 text to the clipboard
/// \param text the null ended UTF8 text
/// \return true if text was copied to clipboard
HORUS_API bool copyToClipboard(const char* text);

/// Paste UTF8 from clipboard
/// \param outText a pointer to a buffer where to store the text, provided by user
/// \param maxTextSize the available text buffer size
/// \return true if text was pasted
HORUS_API bool pasteFromClipboard(char* outText, u32 maxTextSize);

/// \return the current input event which was popped from the event queue
HORUS_API const InputEvent& getInputEvent();

/// Cancel the current event, after this function call the event will be null, so no widget/window will react
HORUS_API void cancelEvent();

/// Add an input event to the queue, usually used by input providers to push events to event queue
HORUS_API void addInputEvent(const InputEvent& event);

/// Signal that the mouse was moved, used by input providers
HORUS_API void setMouseMoved(bool moved);

/// \return the input event count in the event queue
HORUS_API size_t getInputEventCount();

/// \return the input event at the index
/// \param index the event index (maximum is getInputEventCount())
HORUS_API InputEvent getInputEventAt(size_t index);

/// Set the current input event, usually called by input providers
/// \param event the event to be set
HORUS_API void setInputEvent(const InputEvent& event);

/// Clear the input event queue, usually called by input providers
HORUS_API void clearInputEventQueue();

/// Set the current mouse cursor type
/// \param type the cursor type
HORUS_API void setMouseCursor(MouseCursorType type);

/// Create a mouse cursor from a bitmap
/// \param pixels the 32bit color bitmap, RGBA
/// \param width width of the cursor bitmap
/// \param height height of the cursor bitmap
/// \param hotSpotX the cursor pointer hot spot X coordinate, relative to the bitmap size
/// \param hotSpotY the cursor pointer hot spot Y coordinate, relative to the bitmap size
/// \return the created mouse cursor
HORUS_API HMouseCursor createMouseCursor(Rgba32* pixels, u32 width, u32 height, u32 hotSpotX = 0, u32 hotSpotY = 0);

/// Create a mouse cursor from a bitmap loaded from a PNG image file
/// \param hotSpotX the cursor pointer hot spot X coordinate, relative to the bitmap size
/// \param hotSpotY the cursor pointer hot spot Y coordinate, relative to the bitmap size
/// \return the created mouse cursor
HORUS_API HMouseCursor loadMouseCursor(const char* imageFilename, u32 hotSpotX = 0, u32 hotSpotY = 0);

/// Delete a custom mouse cursor
/// \param cursor the cursor to be deleted
HORUS_API void deleteMouseCursor(HMouseCursor cursor);

/// Set the current custom mouse cursor
/// \param cursor the custom mouse cursor to be set
HORUS_API void setMouseCursor(HMouseCursor cursor);

//////////////////////////////////////////////////////////////////////////
// Windowing & docking functions
//////////////////////////////////////////////////////////////////////////

HORUS_API DockNodeId createRootDockNode(HNativeWindow nativeWnd);

HORUS_API void dockLayoutDeleteChildren(DockNodeId rootNodeId);
HORUS_API void dockLayoutSplit(DockNodeId nodeId, DockNodeSplitType splitType, f32 firstNodeSizeUnitPercent, DockNodeId* outNodeId1, DockNodeId* outNodeId2);
HORUS_API void dockLayoutSetNodeWindow(DockNodeId parentNode, const char* windowId);
HORUS_API void dockLayoutRecalculate();

HORUS_API bool beginWindow(const char* windowId, const char* title, Rect* initialRect, HImage img);
HORUS_API void endWindow();
HORUS_API void setWindowVisible(const char* windowId, bool visible);
HORUS_API void setNextWindowFlags(WindowFlags flags);
HORUS_API void focusWindow(const char* windowId);
HORUS_API void debugWindows();
HORUS_API void dockWindow(const char* windowId, const char* targetWindowId, DockType dockType);
HORUS_API void undockWindow(const char* windowId, const Point& windowPos = Point());

HORUS_API void setCurrentNativeWindow(HNativeWindow nativeWnd);
HORUS_API void beginRendering();
HORUS_API void endRendering();

HORUS_API bool isMouseOverWindow();

HORUS_API void setWindowCapture();

HORUS_API void releaseWindowCapture();

/// \return the window client rect
HORUS_API Rect getCurrentWindowClientRect();
/// \return the window client rect, used usually to render custom scenes
HORUS_API Rect getWindowClientRect(const char* windowId);

/// Save the windows docking state
/// \param filename the *.hui filename relative to executable where to save the state
/// \return true if save was ok
HORUS_API bool saveDockingState(const char* filename);
/// Save the docking state to memory, the returned data ptr contains the state info and it is now owned by you
HORUS_API u8* saveDockingStateToMemory(size_t& outStateInfoSize);
//TODO: save docking state to structures too

/// Load the docking state
/// \param filename the *.hui filename relative to executable from where to load the state
/// \return true if the load was ok
HORUS_API bool loadDockingState(const char* filename);
HORUS_API bool loadDockingStateFromMemory(const u8* stateInfo, size_t stateInfoSize);

///////////////////////////////////////////////////////////////////////////////
// Application functions
///////////////////////////////////////////////////////////////////////////////

/// Present the contents of the backbuffer for each OS native window, called after all rendering is done
HORUS_API void present();

/// Present the contents of the backbuffer for a custom OS native window, called after all rendering is done
HORUS_API void presentNativeWindow(HNativeWindow nativeWnd);

/// Shut down the library
HORUS_API void shutdown();

//////////////////////////////////////////////////////////////////////////
// Images
//////////////////////////////////////////////////////////////////////////

/// Load a PNG image from file (it doesn't need to be power of two in dimension) and add it to the theme's image atlas.
/// \param filename the PNG filename, relative to the executable
/// \return the created image or nullptr if it cannot be loaded
HORUS_API HImage loadImage(const char* filename);

/// Create an image from memory
/// \param pixels the RGBA 32bit color pixels buffer
/// \param width the width in pixels
/// \param height the height in pixels
/// \return the created image or nullptr if error
HORUS_API HImage createImage(Rgba32* pixels, u32 width, u32 height);

/// \return an image size as a point (x = width, y = height)
/// \param image the image
HORUS_API Point getImageSize(HImage image);

/// Update an image's pixel data
/// \param image the image to be updated
/// \param pixels the new pixels of the image
HORUS_API void updateImagePixels(HImage image, Rgba32* pixels);

/// Delete an image
/// \param image the image to be deleted
HORUS_API void deleteImage(HImage image);

/// Load an image from a PNG file, it will not add it to the theme's image atlas. Used when you need an image data for something else.
/// \param filename the PNG filename
/// \return the raw image info and data
HORUS_API ImageData loadImageData(const char* filename);

/// Delete a image object after your used/copied its contents
/// \param image the raw image
HORUS_API void deleteImageData(ImageData& image);

//////////////////////////////////////////////////////////////////////////
// Image atlas
//////////////////////////////////////////////////////////////////////////

/// Create a new image atlas. Usually used for collections of images (for making thumbnail browsers for example)
/// \param width the width of the atlas image
/// \param height the height of the atlas image
/// \return the new atlas handle
HORUS_API HAtlas createAtlas(u32 width, u32 height);

/// Delete an image atlas
/// \param atlas the atlas to be deleted
HORUS_API void deleteAtlas(HAtlas atlas);

/// Add an image to an image atlas (it will just queue it, to pack the images into the atlas, call packAtlas)
/// \param atlas the image atlas
/// \param image the raw image to be queued for add
/// \return the new image handle created in the image atlas
HORUS_API HImage addAtlasImage(HAtlas atlas, const ImageData& image);

/// Pack image atlas. This will optimally fit all the queued images into the image atlas. This operation might add new textures to the atlas' texture array if some of the images do not fit inside the current atlas texture(s)
/// \param atlas the atlas to be packed
/// \return true if all queued images were packed ok
HORUS_API bool packAtlas(HAtlas atlas, u32 border = 2);

//////////////////////////////////////////////////////////////////////////
// Themes
//////////////////////////////////////////////////////////////////////////

/// Set the current theme
/// \param theme the theme to be set as current
HORUS_API void setTheme(HTheme theme);

/// \return the current theme
HORUS_API HTheme getTheme();

/// Delete a theme
/// \param theme the theme to be deleted, if this is the current theme it will be set to null
HORUS_API void deleteTheme(HTheme theme);

/// Create a new theme
/// \param atlasTextureSize the width and height of the atlas texture, where theme images are kept
/// \return the newly created theme
HORUS_API HTheme createTheme(u32 atlasTextureSize);

HORUS_API void setThemeUserSetting(HTheme theme, const char* name, const char* value);

HORUS_API const char* getThemeUserSetting(HTheme theme, const char* name);

/// Add a image to a theme's atlas (it will not pack it yet to the atlas, call buildTheme for that)
/// \param theme the theme
/// \param img the image to be added
/// \return the newly created image handle
HORUS_API HImage addThemeImage(HTheme theme, const ImageData& img);

HORUS_API HImage getThemeImage(HTheme theme, const char* imageName);

HORUS_API void setThemeImage(HTheme theme, const char* imageName, HImage image);

HORUS_API void setWidgetStyle(WidgetType widgetType, const char* styleName);

HORUS_API void pushWidgetStyle(WidgetType widgetType, const char* styleName);

HORUS_API void popWidgetStyle();

HORUS_API void setWidgetElementStyle(WidgetElementId widgetElementId, const char* styleName);

HORUS_API void setDefaultWidgetStyle(WidgetType widgetType);

HORUS_API void setDefaultWidgetElementStyle(WidgetElementId widgetElementId);

HORUS_API void setUserWidgetElementStyle(const char* elementName, const char* styleName);

/// Set a theme's widget element info
/// \param theme the theme of the widget element
/// \param elementId the element to be set
/// \param widgetStateType which state to be set
/// \param elementInfo the element info to be set
HORUS_API void setThemeWidgetElement(
	HTheme theme,
	WidgetElementId elementId,
	WidgetStateType widgetStateType,
	const WidgetElementInfo& elementInfo,
	const char* styleName = "default");

/// Build a theme after images were added to its atlas, respectively packing the theme's image atlas
/// \param theme the theme to be built
void buildTheme(HTheme theme);

/// Set a theme's user widget element info
/// \param theme the theme of the widget element
/// \param userElementName the element name
/// \param widgetStateType which state to be set
/// \param elementInfo the element info to be set
HORUS_API void setThemeUserWidgetElement(
	HTheme theme,
	const char* userElementName,
	WidgetStateType widgetStateType,
	const WidgetElementInfo& elementInfo,
	const char* styleName = "default");

/// Return current theme widget element's info
/// \param elementId the widget element id
/// \param state the element state
/// \param outInfo returned element info
HORUS_API void getThemeWidgetElementInfo(WidgetElementId elementId, WidgetStateType state, WidgetElementInfo& outInfo, const char* styleName = "default");

/// Return current theme user widget element's info
/// \param userElementName the user widget element name
/// \param state the element state
/// \param outInfo returned element info
HORUS_API void getThemeUserWidgetElementInfo(const char* userElementName, WidgetStateType state, WidgetElementInfo& outInfo, const char* styleName = "default");

HORUS_API void setThemeWidgetElementParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const char* paramValue);

HORUS_API const char* getThemeWidgetElementStringParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const char* defaultValue = "");

HORUS_API f32 getThemeWidgetElementFloatParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, f32 defaultValue = 0.0f);

HORUS_API const Color& getThemeWidgetElementColorParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const Color& defaultValue = Color());

HORUS_API void setThemeUserWidgetElementParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const char* paramValue);

HORUS_API const char* getThemeUserWidgetElementStringParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const char* defaultValue = "");

HORUS_API f32 getThemeUserWidgetElementFloatParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, f32 defaultValue = 0.0f);

HORUS_API const Color& getThemeUserWidgetElementColorParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const Color& defaultValue = Color());

/// Create a new font object
/// \param theme the theme where to place the font
/// \param name the name of the font (a given name like for example: 'smallItalic')
/// \param fontFilename the TTF/OTF font filename, relative to executable
/// \param faceSize the font face size in font units
/// \return the newly created font handle
HORUS_API HFont createThemeFont(HTheme theme, const char* name, const char* fontFilename, u32 faceSize);

/// Release font reference, if font usage is zero, the font is deleted
/// \param font the font to be reference released
HORUS_API void releaseThemeFont(HTheme theme, HFont font);

/// \return the font by name, from the current theme
/// \param themeFontName the name of the font as it is in the theme
HORUS_API HFont getFont(const char* themeFontName);

/// \return the font by name, from the specified theme
HORUS_API HFont getThemeFont(HTheme theme, const char* themeFontName);

//////////////////////////////////////////////////////////////////////////
// Layouts
//////////////////////////////////////////////////////////////////////////

/// Begin a layout area, an invisible rectangle on the current window area where widgets will be laid out
HORUS_API void beginLayout(const Rect& rect);
HORUS_API void endLayout();
HORUS_API void pushId(const char* id);
HORUS_API void pushId(u32 id);
HORUS_API void pushId(void* id);
HORUS_API void popId();
HORUS_API void pushLayout();
HORUS_API void popLayout();

/// Begin a layout made up as columns which can have percentage based widths or fixed
/// \param columnCount the number of columns to be created
/// \param preferredWidths a float array of the preferred width for each columns, if width is smaller of equal to 1.0f it is considered a percentage of the parent layout, if it is greater than 1.0f it is considered a fixed pixel size
/// \param minWidths a float array of the minimal width for each columns, if width is smaller of equal to 1.0f it is considered a percentage of the parent layout, if it is greater than 1.0f it is considered a fixed pixel size
/// \param maxWidths a float array of the maximum width for each columns, if width is smaller of equal to 1.0f it is considered a percentage of the parent layout, if it is greater than 1.0f it is considered a fixed pixel size
HORUS_API void beginColumns(u32 columnCount, const f32 preferredWidths[] = nullptr, const f32 minWidths[] = nullptr, const f32 maxWidths[] = nullptr);

/// Begin an equal widths array of columns
/// \param columnCount the column count
/// \param minWidths a float array of the minimal width for each columns, if width is smaller of equal to 1.0f it is considered a percentage of the parent layout, if it is greater than 1.0f it is considered a fixed pixel size
/// \param addPadding true if you want padding to be added to left and right sides of the columns group
HORUS_API void beginEqualColumns(u32 columnCount, const f32 minWidths[] = nullptr, const f32 maxWidths[] = nullptr);

/// Begin a two columns layout
HORUS_API void beginTwoColumns();

/// Begin a three columns layout
HORUS_API void beginThreeColumns();

/// Begin a four columns layout
HORUS_API void beginFourColumns();

/// Begin a five column layout
HORUS_API void beginFiveColumns();

/// Begin a six column layout
HORUS_API void beginSixColumns();

/// Advance to next column in the current column layout
HORUS_API void nextColumn();

/// \return the current column's rectangle in coordinates relative to current window
HORUS_API Rect getColumnRect();

/// End the columns layout (does the same thing as nextColumn, for the last column)
HORUS_API void endColumns();

/// Draw a column header widget, usually called inside a column layout. This widget might get resized with mouse, so its parent column could get resized
/// \param label the label of the header
/// \param preferredWidth the normal width of the column header
/// \param minWidth the minimal width of the column header
/// \param maxWidth the maximal width of the column header
HORUS_API void columnHeader(const char* label, f32 width, f32 preferredWidth, f32 minWidth, f32 maxWidth);

/// Begin a scroll view area widget
/// \param height the height of the scroll area
/// \param scrollPosition the current scroll position (given by endScrollView)
/// \param virtualHeight the virtual inside scroll height, if its zero then its automatically calculated from the child widgets inside this area
HORUS_API void beginScrollView(f32 height, f32 scrollPosition, f32 virtualHeight = 0.0f);

/// Ends a scroll view area widget
/// \return the current scroll position (offset)
HORUS_API f32 endScrollView();

/// Begin a virtual list content area, used for many items, inside the beginScrollView/endScrollView
/// \param totalRowCount the number of rows
/// \param itemHeight the height of one item
/// \param scrollPosition the current scroll offset of the scroll view widget
HORUS_API void beginVirtualListContent(u32 totalRowCount, u32 itemHeight, f32 scrollPosition);

/// End a virtual list content area
HORUS_API void endVirtualListContent();

/// Push the old padding and set a new one, padding is the left and right side horizontal spacing for widgets
/// \param newPadding the new horizontal padding value
HORUS_API void pushPadding(PaddingType type, const Point& newPadding);
HORUS_API void pushWidgetPadding(const Point& newPadding);

/// Pop the previous padding value from stack and set it as current
HORUS_API void popPadding(PaddingType type);
HORUS_API void popWidgetPadding();

/// Push a new padding for column content
HORUS_API void pushColumnPadding(f32 newPadding);

HORUS_API void popColumnPadding();

/// Push the old spacing value to stack and set a new spacing value, spacing is the vertical space between widgets
/// \param newSpacing the new vertical spacing value
HORUS_API void pushSpacing(f32 newSpacing);

/// Pop old spacing value from stack and set it as current
HORUS_API void popSpacing();

/// Push the old spacing value to stack and set a new spacing value, spacing is the vertical space between widgets
/// \param newSpacing the new vertical spacing value
HORUS_API void pushColumnSpacing(f32 newSpacing);

/// Pop old spacing value from stack and set it as current
HORUS_API void popColumnSpacing();

/// \return the current vertical spacing value
HORUS_API f32 getSpacing();

HORUS_API f32 getColumnSpacing();

/// \return the current horizontal left and right side padding value
HORUS_API const Point& getPadding(PaddingType type);

// Handy version to get widget padding
HORUS_API const Point& getWidgetPadding();

HORUS_API f32 getColumnPadding();

/// Set the global UI scale, this will scale all the elements from widgets to text
/// \param scale a value, use with consideration, will regenerate font atlas, slow
HORUS_API void changeScale(f32 scale);

/// \return the current global UI scale
HORUS_API f32 getScale();

/// Push and set a new tinting color on stack, to colorize the next widget on specific parts
/// \param color the tint color
/// \param type what elements of the widget to tint
HORUS_API void pushTint(const Color& color, TintColorType type = TintColorType::All, TintColorOpType opType = TintColorOpType::Multiply);

/// Pop the old tint color from stack
HORUS_API void popTint();

HORUS_API Color applyTint(const Color& originalColor, TintColorType type);

/// Draw a delayed tooltip widget near the previous widget
/// \param text the label of the tooltip
/// \return true if the tooltip is visible now
HORUS_API bool tooltip(const char* text);

/// Begin drawing a custom tooltip (delayed), which contains other widgets like image and labels etc.
/// \param width the tooltip width
/// \return true if the tooltip is visible now
HORUS_API bool beginCustomTooltip(f32 width);

/// End drawing a custom tooltip
HORUS_API void endCustomTooltip();

/// Begin draw a box, which may contain other widgets
/// \param color the box tint color
/// \param widgetElementId the widget element id image to use when drawing the box
/// \param state the widget element state to draw with
/// \param customHeight a forced custom height, otherwise auto calculated from the total height the child widgets have
HORUS_API void beginBox(
	const Color& color,
	WidgetElementId widgetElementId = WidgetElementId::BoxBody,
	WidgetStateType state = WidgetStateType::Normal,
	f32 customHeight = 0.0f);

/// Begin draw a box, which may contain other widgets
/// \param color the box tint color
/// \param userElementName the user widget element name whose image to use when drawing the box
/// \param state the widget element state to draw with
/// \param customHeight a forced custom height, otherwise auto calculated from the total height the child widgets have
HORUS_API void beginBox(
	const Color& color,
	const char* userElementName,
	WidgetStateType state = WidgetStateType::Normal,
	f32 customHeight = 0.0f);

/// End the box widget
HORUS_API bool endBox();

/// Begin drawing a modal popup widget on top of all other popups or widgets
/// \param width the width of the popup
/// \param flags the popup flags
/// \param position when custom position, this is the window coordinates of the popup
/// \param widgetElementId will use this element's theme to draw the popup body
HORUS_API void beginPopup(
	const char* id,
	f32 width,
	PopupFlags flags = PopupFlags::BelowLastWidget,
	const Point& position = Point(),
	WidgetElementId widgetElementId = WidgetElementId::PopupBody);

/// End a popup widget
HORUS_API void endPopup();

/// Close the current popup, used inside begin/endPopup
HORUS_API void closePopup();

/// \return true if the popup must be closed, due to user input, used inside begin/endPopup
HORUS_API bool mustClosePopup();

/// \return true if the user clicked outside popup's rect, used inside begin/endPopup
HORUS_API bool clickedOutsidePopup();

/// \return true if mouse is inside popup's rect, used inside begin/endPopup
HORUS_API bool mouseOutsidePopup();

/// \return true if the user pressed escape while current popup is active, used inside begin/endPopup
HORUS_API bool pressedEscapeOnPopup();

/// Draw a message box popup
/// \param title the message box title
/// \param message the message
/// \param buttons the visible buttons flags in the message box
/// \param img the image of the message box
/// \param width the width of the message box
/// \param customImg the custom image, if set in the image param
/// \return the pushed button in the message box
HORUS_API MessageBoxButtons messageBox(
	const char* title,
	const char* message,
	MessageBoxButtons buttons = MessageBoxButtons::Ok,
	MessageBoxImage img = MessageBoxImage::Info,
	u32 width = 400,
	HImage customImg = 0);

/// Set the next widget as enabled or not
/// \param enabled if true, the widget is enabled for input
HORUS_API void setNextEnabled(bool enabled);

/// Set next widget as focused
HORUS_API void setNextFocused();

/// Draw a button widget
/// \param label the button text
/// \return true if button was pressed
HORUS_API bool button(const char* label);

/// Draw a button with an image on it
/// \param img the image
/// \param height the button height, if zero then it takes the image's height
/// \param down if true the button is in the pressed state
/// \return true if the button was pressed
HORUS_API bool imageButton(HImage img, f32 width, f32 height, HImage disabledImg = 0, bool down = false);

/// Draw a text input widget
/// \param text the text to be edited, provided by user
/// \param maxTextSize the max size of the text buffer
/// \param valueType the value type filter, what value is allowed in the text
/// \param defaultText the grayed default text when there is no text value
/// \param img the image drawn in the widget
/// \return true if the text was modified
HORUS_API bool textInput(char* text, u32 maxTextSize, TextInputValueMode valueType = TextInputValueMode::Any, const char* defaultText = nullptr, HImage img = 0, bool password = false, const char* passwordChar = "\95");

/// Draw an integer number slider widget
/// \param minVal the minimum value
/// \param maxVal the maximum value
/// \param value the value ref
/// \param useStep use stepping when moving slider
/// \param step if useStep is true, then this is the step size
/// \return true if value was modified
HORUS_API bool sliderInteger(const char* id, i32 minVal, i32 maxVal, i32& value, bool useStep = false, i32 step = 0);

/// Draw a float number slider widget
/// \param minVal the minimum value
/// \param maxVal the maximum value
/// \param value the value ref
/// \param useStep use stepping when moving slider
/// \param step if useStep is true, then this is the step size
/// \return true if value was modified
HORUS_API bool sliderFloat(const char* id, f32 minVal, f32 maxVal, f32& value, bool useStep = false, f32 step = 0);


HORUS_API bool comboSliderInteger(i32* value, f32 stepsPerPixel = 1.0f, i32 arrowStep = 1, const char* formatStr = nullptr);
HORUS_API bool comboSliderIntegerRanged(i32* value, i32 minVal, i32 maxVal, f32 stepsPerPixel = 1, i32 arrowStep = 1.0f, const char* formatStr = nullptr);
HORUS_API bool comboSliderFloat(f32* value, f32 stepsPerPixel = 1.0f, f32 arrowStep = 1.0f, const char* formatStr = nullptr);
HORUS_API bool comboSliderFloatRanged(f32* value, f32 minVal, f32 maxVal, f32 stepsPerPixel = 1.0f, f32 arrowStep = 1.0f, const char* formatStr = nullptr);
HORUS_API bool rotarySliderFloat(const char* label, f32* value, f32 minVal, f32 maxVal, f32 step, bool twoSide = false, f32 fineStepDivideFactor = 10.f);

/// Draw a image widget
/// \param image the image to draw
/// \param height the height of the image, if zero then the actual image height will be used
/// \param horizontalAlign the horizontal image align mode
/// \param verticalAlign the vertical image align mode
/// \param fit how the image is fitted in the rectangle, resize mode
/// \return true if it was clicked on
HORUS_API bool image(HImage image, f32 height = 0, HAlignType horizontalAlign = HAlignType::Center, VAlignType verticalAlign = VAlignType::Center, ImageFitType fit = ImageFitType::KeepAspect);

/// Draw a progress bar widget
/// \param value the progress as a percentage
HORUS_API void progress(f32 value, f32 maxValue = 0.0f, bool showText = false, bool showRealValues = true, const char* indeterminateText = nullptr);

/// Draw a check box widget
/// \param label the check's label
/// \param checked true if it has check mark on
/// \return true if it was changed, result put in checked
HORUS_API bool check(const char* label, bool* checkVar);

/// Draw a radio box widget
/// \param label the radio's label
/// \param currentRadioValue location of the current value of the radio group
/// \param thisValue the value of this radio button
/// \return true if it was changed, result put in checked
HORUS_API bool radio(const char* label, i32* currentRadioValue, i32 thisValue);

/// Draw a label text widget
/// \param label the label's text
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HORUS_API bool label(const char* label, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a label text widget with a custom font
/// \param label the label's text
/// \param font the label's font
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HORUS_API bool labelCustomFont(const char* label, HFont font, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a multiline label text widget (involves more logic than a single lined label)
/// \param label the label's text
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HORUS_API bool labelMultiline(const char* label, HAlignType horizontalAlign);

/// Draw a multiline label text widget with a custom font (involves more logic than a single lined label)
/// \param label the label's text
/// \param font the label's font
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HORUS_API bool labelCustomFontMultiline(const char* label, HFont font, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a expandable panel widget
/// \param label the text of the panel
/// \param expandedVar keeps true if the panel is expanded
/// \return true if the panel state changed
HORUS_API bool expandable(const char* label, bool* expandedVar = nullptr);

/// Draw a dropdown widget
/// \param selectedIndex the current selected item index
/// \param items an array of strings for the items
/// \param itemCount the number of items in the list
/// \param maxVisibleDropDownItems the maximum number of visible items in the drop down list, if ~0 then its automatic
/// \return true if it the selection changed
HORUS_API bool dropdown(const char* id, i32& selectedIndex, const char** items, u32 itemCount, u32 maxVisibleDropDownItems = ~0);

HORUS_API bool list(i32* selectedIndices, u32 maxSelectedIndices, ListSelectionMode selectionType, const char** items, u32 itemCount);

/// Draw a selectable label
/// \param label the selectable's text
/// \param stateFlags the state of the selectable widget
/// \return true if it is selected
HORUS_API bool selectable(const char* label, SelectableFlags stateFlags = SelectableFlags::Normal);

/// Draw a selectable label with custom font
/// \param label the selectable's text
/// \param font the label's text font
/// \param stateFlags the state of the selectable widget
/// \return true if it is selected
HORUS_API bool selectableCustomFont(const char* label, HFont font, SelectableFlags stateFlags = SelectableFlags::Normal);

//////////////////////////////////////////////////////////////////////////
// Separators
//////////////////////////////////////////////////////////////////////////

/// Draw a horizontal line widget
HORUS_API void line();

/// Leave a normal space between previous widget and next one
HORUS_API void space(f32 customSpacing = 0.0f);

/// Make next widget show on the same row as the last widget. The widget width depends on the widget type, the content inside it, etc.
/// Not all widgets support the same line modifier, since some need content
HORUS_API void beginSameLine(f32 spacing = 0.0f);

HORUS_API void endSameLine();

HORUS_API void pushSameLineSpacing(f32 horizontalSpace = 0.0f);

HORUS_API f32 popSameLineSpacing();

HORUS_API void pushWidth(f32 width);

HORUS_API f32 popWidth();
HORUS_API void setNextWidth(f32 width);

/// Begin a custom user viewport area
/// \param height the height of the viewport, if zero, it will take the entire remaining container height
/// \return the rectangle in window coordinates of the actual viewport area, use this to draw your custom things in
HORUS_API Rect beginViewport(f32 height = 0);

/// End the current user viewport
HORUS_API void endViewport();

//////////////////////////////////////////////////////////////////////////
// Menus
//////////////////////////////////////////////////////////////////////////

/// Begin a menu bar widget
HORUS_API bool beginMenuBar();

/// End the current menu bar widget
HORUS_API void endMenuBar();

/// Begin a menu panel widget (it will show up only when clicked)
/// \param label the menu text
/// \param flags the menu flags
/// \return true if the menu is visible, use it in a if() statement to show menu items
HORUS_API bool beginMenu(const char* label, SelectableFlags flags = SelectableFlags::Normal);

/// End the current menu
HORUS_API void endMenu();

/// Begin drawing a context menu which will open on right click on the previous widget
/// \return true if the menu is opened/visible
HORUS_API bool beginContextMenu(ContextMenuFlags flags = ContextMenuFlags::None);

/// End the current context menu
HORUS_API void endContextMenu();

/// Draw a menu item widget, use inside begin/end menu (or context menu)
/// \param label the menu item text
/// \param shortcut the key shortcut text
/// \param img the menu item left side image
/// \param flags the menu item flags
/// \return true if the menu item was clicked on
HORUS_API bool menuItem(const char* label, const char* shortcut = "", HImage img = 0, SelectableFlags flags = SelectableFlags::Normal);

/// Draw a menu item separator
HORUS_API void menuSeparator();

//////////////////////////////////////////////////////////////////////////
// Dockable Tabs
//////////////////////////////////////////////////////////////////////////

/// Start a tab group
/// \param selectedIndex the selected tab index
HORUS_API void beginTabGroup(TabIndex selectedIndex);

/// Draw a tab widget
/// \param label the text of the tab
/// \param img the image of the tab
HORUS_API void tab(const char* label, HImage img);

/// End the tab group
HORUS_API TabIndex endTabGroup();

//////////////////////////////////////////////////////////////////////////
// Immediate state query for the last widget
//////////////////////////////////////////////////////////////////////////

/// \return true if the previous widget is hovered
HORUS_API bool isHovered();

/// \return true if the previous widget is focused
HORUS_API bool isFocused();

/// \return true if the previous widget is pressed down
HORUS_API bool isPressed();

/// \return true if the previous widget is clicked
HORUS_API bool isClicked();

/// \return true if the previous widget is visible
HORUS_API bool isVisible();

/// \return true if the change for the widget's value ended, used for undo systems to add the undo action only after the drag/edit ended
HORUS_API bool isChangeEnded();

/// \return the current widget id (the next widget's id)
HORUS_API WidgetId getWidgetId();

/// \return the current mouse position inside current window
HORUS_API Point getMousePosition();

//////////////////////////////////////////////////////////////////////////
// Drag and drop logic support
//////////////////////////////////////////////////////////////////////////

/// \return true if there is a drag intent
HORUS_API bool wantsToDragDrop();

/// set the mouse cursor to be used when dropping allowed
/// \param dropAllowedCursor the mouse cursor
HORUS_API void setDragDropMouseCursor(HMouseCursor dropAllowedCursor);

/// Begin dragging an object
/// \param dragObjectUserType the user type for the object
/// \param dragObject the user object to drag as payload
HORUS_API void beginDragDrop(u32 dragObjectUserType, void* dragObject);

/// End drag and drop operation
HORUS_API void endDragDrop();

/// Allow drag drop for the next widgets
HORUS_API void allowDragDrop();

/// Disallow drop for the next widgets
HORUS_API void disallowDragDrop();

/// \return true if the user dropped payload on previous widget
HORUS_API bool droppedOnWidget();

/// \return the drag drop payload user object pointer
HORUS_API void* getDragDropObject();

/// \return the drag drop payload user object type
HORUS_API u32 getDragDropObjectType();

//////////////////////////////////////////////////////////////////////////
// Custom widgets
//////////////////////////////////////////////////////////////////////////

/// Begin drawing a custom widget
/// \param height the widget height
/// \return the widget rectangle in window coordinates
HORUS_API Rect beginCustomWidget(const char* id, f32 height = 0.0f);

/// End custom widget drawing
HORUS_API void endCustomWidget();

/// Set the next widget position
HORUS_API void setPosition(const Point& position);

/// \return the current widget drawing position
HORUS_API Point getPosition();

HORUS_API void pushPosition();
HORUS_API void popPosition();

/// Increment the widget layer index, the highest layer index will be the active one
HORUS_API void incrementLayerIndex();

/// \return the layer index, after decrementing it
HORUS_API u32 decrementLayerIndex();

///
HORUS_API void decrementWindowMaxLayerIndex();

///
HORUS_API Point getLayoutSize();

///
HORUS_API Rect getWidgetRect();

///
HORUS_API void pushDrawCommandIndex();

///
HORUS_API u32 popDrawCommandIndex();

///
HORUS_API void beginInsertDrawCommands(u32 atIndex);

///
HORUS_API void endInsertDrawCommands();

///
HORUS_API void setFont(HFont font);
HORUS_API void pushFont(HFont font);
HORUS_API void popFont();

///
HORUS_API void setColor(const Color& color);

///
HORUS_API void setLineColor(const Color& color);

///
HORUS_API void setFillColor(const Color& color);

///
HORUS_API void drawTextAt(const char* text, const Point& position);

///
HORUS_API void drawTextInBox(const char* text, const Rect& rect, HAlignType horizontalAlign, VAlignType verticalAlign);

///
HORUS_API Point getTextSize(const char* text);

///
HORUS_API void drawImage(HImage image, const Point& position, f32 scale);

///
HORUS_API void drawStretchedImage(HImage image, const Rect& rect);

///
HORUS_API void drawBorderedImage(HImage image, u32 border, const Rect& rect);

///
HORUS_API void setLineStyle(const LineStyle& style);

///
HORUS_API void setFillStyle(const FillStyle& style);

///
HORUS_API void drawLine(const Point& a, const Point& b);

///
HORUS_API void drawPolyLine(const Point* points, u32 pointCount, bool closed = false);

///
HORUS_API void drawCircle(const Point& center, f32 radius, u32 segments = 32);

///
HORUS_API void drawEllipse(const Point& center, f32 radiusX, f32 radiusY, u32 segments = 32);

///
HORUS_API void drawRectangle(const Rect& rc);

///
HORUS_API void drawSolidRectangle(const Rect& rc);

///
HORUS_API void drawSpline(SplineControlPoint* points, u32 count, f32 segmentSize = 15);

///
HORUS_API void drawArrow(const Point& startPoint, const Point& endPoint, f32 tipLength, f32 tipWidth, bool drawBodyLine = true);

HORUS_API void drawSolidTriangle(const Point& p1, const Point& p2, const Point& p3);

//////////////////////////////////////////////////////////////////////////
// Utility panels and complex/combined mega-widgets
//////////////////////////////////////////////////////////////////////////

/// Draw a color picker popup widget
HORUS_API bool colorPicker(const char* id, Color* inOutColor, ColorPickerFlags flags = (ColorPickerFlags)0, const Color* oldColor = nullptr);

/// Draw a 3D double vector editor widget
HORUS_API bool vec3Editor(const char* id, f64& x, f64& y, f64& z, f64 scrollStep = 0.03f);

/// Draw a 3D float vector editor widget
HORUS_API bool vec3Editor(const char* id, f32& x, f32& y, f32& z, f32 scrollStep = 0.03f);

/// Draw a 2D double vector editor widget
HORUS_API bool vec2Editor(const char* id, f64& x, f64& y, f64 scrollStep = 0.03f);

/// Draw a 2D float vector editor widget
HORUS_API bool vec2Editor(const char* id, f32& x, f32& y, f32 scrollStep = 0.03f);

/// Draw an object reference editor
HORUS_API bool objectRefEditor(const char* id, HImage targetImg, HImage clearImg, const char* objectTypeName, const char* valueAsString, u32 objectType, void** outObject, bool* objectValueWasModified);

//////////////////////////////////////////////////////////////////////////
// System native file dialogs
//////////////////////////////////////////////////////////////////////////

/// Show a native open file dialog
HORUS_API bool openFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize);

/// Show a native open multiple file dialog
HORUS_API bool openMultipleFileDialog(const char* filterList, const char* defaultPath, OpenMultipleFileSet& outPathSet);

/// Show a native save file dialog
HORUS_API bool saveFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 maxOutPathSize);

/// Show a native pick folder dialog
HORUS_API bool pickFolderDialog(const char* defaultPath, char* outPath, u32 maxOutPathSize);

//////////////////////////////////////////////////////////////////////////
// Utility functions
//////////////////////////////////////////////////////////////////////////

/// Convert an int value to string
HORUS_API void toStringI32(i32 value, char* outString, u32 outStringMaxSize, u32 fillerZeroesCount = 0);

/// Convert a float value to string
HORUS_API void toStringF32(f32 value, char* outString, u32 outStringMaxSize, i32 decimalPlaces = ~0);

HORUS_API Color getColorFromText(const char* colorText);
HORUS_API Color colorFromHex(const char* hexText);
HORUS_API u32 intColorFromHex(const char* hexText);
HORUS_API std::string colorToHex(const Color& color);
HORUS_API std::string intColorToHex(const u32 color);
HORUS_API Color hsvToRgb(const Color& hsv);
HORUS_API Color rgbToHsv(const Color& rgb);
HORUS_API Color hueToRgb(f32 hue, f32 alpha = 1.0f);

}
/** @}*/
