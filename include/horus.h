#pragma once
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <vector>

/*
------------------------------------------------------------------------------
	Horus UI
------------------------------------------------------------------------------
	Immediate Mode Graphical User Interface Library

	(C) All rights reserved 2016-2017 7thFACTOR Software - Nicusor Nedelcu (nekitu)
------------------------------------------------------------------------------
*/

/// \file horus.h

#ifdef HORUS_CUSTOM_CONFIG_FILE
#include HORUS_CUSTOM_CONFIG_FILE
#else
#include "horus_config.h"
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
	#define HORUS_CLASS_API
#else 
#ifdef _WINDOWS
	#ifdef HORUS_EXPORT
		#define HORUS_API extern "C++" __declspec(dllexport)
		#define HORUS_CLASS_API __declspec(dllexport)
	#else
		#ifdef HORUS_IMPORT
			#define HORUS_API extern "C++" __declspec(dllimport)
			#define HORUS_CLASS_API __declspec(dllimport)
		#else
			#define HORUS_API
			#define HORUS_CLASS_API
		#endif
	#endif
#else
	#ifdef HORUS_EXPORT
		#define HORUS_API __attribute__((dllexport))
		#define HORUS_CLASS_API __attribute__((dllexport))
	#else
		#ifdef HORUS_IMPORT
			#define HORUS_API __attribute__((dllimport))
			#define HORUS_CLASS_API __attribute__((dllimport))
		#else
			#define HORUS_API
			#define HORUS_CLASS_API
		#endif
	#endif
#endif
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
#define HORUS_FILE hui::getContextSettings().providers.file
#define HORUS_GFX hui::getContextSettings().providers.gfx
#define HORUS_INPUT hui::getContextSettings().providers.input
#define HORUS_UTF hui::getContextSettings().providers.utf
#define HORUS_IMAGE hui::getContextSettings().providers.image
#define HORUS_FONT hui::getContextSettings().providers.font
#define HORUS_RECTPACK hui::getContextSettings().providers.rectPack

typedef void* HImage;
typedef void* HTheme;
typedef void* HAtlas;
typedef void* HFont;
typedef void* HThemeWidgetElement;
typedef void* HWindow;
typedef void* HViewPane;
typedef void* HViewPaneTab;
typedef void* HViewContainer;
typedef void* HMouseCursor;
typedef void* HGraphicsApiContext;
typedef void* HGraphicsApiTexture;
typedef void* HGraphicsApiRenderTarget;
typedef void* HGraphicsApiVertexBuffer;
typedef void* HContext;
typedef u32 Rgba32;
typedef u32 TabIndex;
typedef u32 ViewId;
typedef u32 GlyphCode;
typedef std::vector<GlyphCode> UnicodeString;

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

/// Font style type
enum class FontStyle
{
	Normal,
	Bold,
	Italic,
	BoldItalic,

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
	IconButton,
	TextInput,
	Slider,
	Progress,
	Image,
	Check,
	Radio,
	Label,
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
	ViewPane,
	MsgBox,
	Box,
	Toolbar,
	ToolbarButton,
	ToolbarDropdown,
	ToolbarSeparator,
	ColumnsHeader,
	ComboSlider,
	RotarySlider,

	Count
};

/// Current supported widget element types, used for themes
enum class WidgetElementId
{
	None = 0,
	Custom,
	WindowBody,
	ButtonBody,
	CheckBody,
	CheckMark,
	RadioBody,
	RadioMark,
	LineBody,
	LabelBody,
	PanelBody,
	PanelCollapsedArrow,
	PanelExpandedArrow,
	TextInputBody,
	TextInputCaret,
	TextInputSelection,
	TextInputDefaultText,
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
	ViewPaneDockRect,
	ViewPaneDockDialRect,
	MenuBarBody,
	MenuBarItem,
	MenuBody,
	MenuItemSeparator,
	MenuItemBody,
	MenuItemShortcut,
	MenuItemCheckMark,
	MenuItemNoCheckMark,
	SubMenuItemArrow,
	MessageBoxIconError,
	MessageBoxIconInfo,
	MessageBoxIconQuestion,
	MessageBoxIconWarning,
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
	RotarySliderBody,
	RotarySliderMark,
	RotarySliderValueDot,

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

enum class TabGroupState
{
	Auto,
	Disabled
};

enum class TabState
{
	Auto,
	Disabled
};

/// When pushTint is called, specifies what element is color tinted
enum class TintColorType
{
	Body,
	Text,
	All,

	Count
};

/// Window flags
enum class WindowFlags
{
	Resizable = HORUS_BIT(0),
	Fixed = HORUS_BIT(1),
	ResizableNoTitle = HORUS_BIT(2),
	FixedNoTitle = HORUS_BIT(3),
	NoTaskbarButton = HORUS_BIT(4),
	Centered = HORUS_BIT(5),
	CustomPosition = HORUS_BIT(6)
};

HORUS_ENUM_AS_FLAGS(WindowFlags);

/// Native window state
enum class WindowState
{
	Normal,
	Maximized,
	Minimized,
	Hidden
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
	Hand,
	FingerPoint,
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

enum class AntiAliasing
{
	None,
	MSAA4X,
	MSAA8X,
	MSAA16X,

	Count
};

/// Docking modes for the view panes
enum class DockType
{
	Left, /// will dock pane to left
	Right, /// will dock pane to right
	Top, /// will dock pane to top
	Bottom, /// will dock pane to bottom
	RootLeft, /// will dock pane to left of root dock space
	RootRight, /// will dock pane to right of root dock space
	RootTop, /// will dock pane to top of root dock space
	RootBottom, /// will dock pane to bottom of root dock space
	TopAsViewTab, /// will dock pane as full pane in the pane tabs bar
};

/// Common message box icons
enum class MessageBoxIcon
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

enum class ToolbarDirection
{
	Horizontal,
	Vertical
};

enum class ContextMenuFlags
{
	None = 0,
	AllowLeftClickOpen = HORUS_BIT(1)
};
HORUS_ENUM_AS_FLAGS(ContextMenuFlags);

enum class PopupFlags : u32
{
	None = 0,
	FadeWindowContents = HORUS_BIT(1), /// fade the contents behind the popup when shown
	WindowCenter = HORUS_BIT(2), /// center the popup to window
	BelowLastWidget = HORUS_BIT(3), /// position the popup below last widget
	RightSideLastWidget = HORUS_BIT(4), /// position the popup on right side of the last widget
	CustomPosition = HORUS_BIT(5), /// use custom popup position
	SameLayer = HORUS_BIT(6), /// internal: don't increment layer index
	TopMost = HORUS_BIT(7), /// set to have this popup top most
	IsMenu = HORUS_BIT(8) /// internal, when this popup is a menu
};
HORUS_ENUM_AS_FLAGS(PopupFlags);

/// A 2D point
class Point
{
public:
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

		return (f32)(x * other.x + y * other.y) / sqrt(m);
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

		return (fabs(u1 - u2) <= tolerance);
	}

	inline bool isAlmosEqual(const Point& other, f32 tolerance = 0.001f) const
	{
		return fabs(x - other.x) <= tolerance
			&& fabs(y - other.y) <= tolerance;
	}

	inline f32 getLength() const
	{
		return sqrt(x * x + y * y);
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

	inline Rect expand(f32 amount)
	{
		return {
			x - amount,
			y - amount,
			width + 2.0f * amount,
			height + 2.0f * amount 
		};
	}

	inline Rect contract(f32 amount)
	{
		return expand(-amount);
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

	inline bool operator != (const Rect& other) const
	{
		constexpr f32 epsilon = 0.00001f;
		return fabs(x - other.x) > epsilon
			|| fabs(y - other.y) > epsilon
			|| fabs(width - other.width) > epsilon
			|| fabs(height - other.height) > epsilon;
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
		WindowResize,
		WindowGotFocus,
		WindowLostFocus,
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
		HWindow window = 0;
	};

	union
	{
		MouseData mouse;
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
	HWindow window = 0;
};

struct HORUS_CLASS_API Color
{
	Color() {}
	Color(u32 color)
	{
		setFromRgba(color);
	}

	Color(f32 R, f32 G, f32 B, f32 A)
		: r(R), g(G), b(B), a(A)
	{}

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

	~OpenMultipleFileSet()
	{
		delete[] filenameBuffer;
		delete[] bufferIndices;
	}
};

/// A view handler is used by the docking system to delegate UI rendering to the user
/// It calls various functions at specific times so the user will just show the UI
struct ViewHandler
{
	/// Called when the user must render the main menu of the application on the specified window
	/// \param window the window for which the main menu to be rendered
	virtual void onTopAreaRender(HWindow window) {}
	virtual void onLeftAreaRender(HWindow window) {}
	virtual void onRightAreaRender(HWindow window) {}
	virtual void onBottomAreaRender(HWindow window) {}
	/// Called when the user must render the widgets for a specific view
	/// \param window the window where the drawing of UI will occur
	/// \param viewPane the view pane where the drawing of UI will occur
	/// \param activeViewId the view ID for which to draw the UI (there can be multiple views with the same ID), data driven UI
	/// \param userDataId the user data ID, which was set by the user for this particular view instance
	virtual void onViewRender(HWindow window, HWindow viewPane, ViewId activeViewId, u64 userDataId) {}
	/// Called when a view was closed
	/// \param window the window where the view pane was closed
	/// \param viewPane the view pane
	/// \param activeViewId the view ID for which to draw the UI (there can be multiple views with the same ID), data driven UI
	/// \param userDataId the user data ID, which was set by the user for this particular view instance
	virtual void onViewClosed(HWindow window, HWindow viewPane, ViewId activeViewId, u64 userDataId) {}
	/// Called just before the frame starts to render
	/// \param window the window where rendering will happen
	virtual void onBeforeFrameRender(HWindow wnd) {}
	/// Called after the frame starts to render
	/// \param window the window where rendering did happen
	virtual void onAfterFrameRender(HWindow wnd) {}
	//TODO: abstract file save interface
	virtual void onViewPaneTabSave(HViewPaneTab tab, u64 dataId, FILE* file) {}
	virtual void onViewPaneTabLoad(HViewPaneTab tab, u64 dataId, FILE* file) {}
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
	HGraphicsApiTexture texture;
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
	struct UtfProvider* utf = 0;
	struct FontProvider* font = 0;
	struct RectPackProvider* rectPack = 0;
};

/// Various HorusUI per-context global settings
struct ContextSettings
{
	ServiceProviders providers;
	TextCachePruneMode textCachePruneMode = TextCachePruneMode::Time; /// how to prune the unicode text cache which is not used for a while
	f32 textCachePruneMaxTimeSec = 5; /// after this time, if an Unicode text is not accessed, it's discarded from cache, textCachePruneMode must be Time
	f32 textCachePruneMaxFrames = 500; /// after this frame count, if an Unicode text is not accessed, it's discarded from cache, textCachePruneMode must be Frames
	f32 textCachePruneIntervalSec = 5; /// after each interval has passed, the pruning of unused texts is executed, will delete the texts that were not used for the last textCachePruneMaxTimeMs or textCachePruneMaxFrames, depending on the prune mode
	u32 defaultAtlasSize = 4096; /// default atlas textures size in pixels
	SliderDragDirection sliderDragDirection = SliderDragDirection::Any; /// allows to change slider value from any direction drag, vertical or horizontal
	bool sliderInvertVerticalDragAmount = false; /// if true and vertical sliding allowed, it will invert the drag amount
	f32 dragStartDistance = 3; /// the max distance after which a dragging operation starts to occur when mouse down and moved, in pixels
	f32 whiteImageUvBorder = 0.001f; /// this value is subtracted from the white image used to draw lines, to avoid black border artifacts
	f32 sameLineHeight = 20.0f; /// the height of a line when sameLine() is used to position widgets on a single row/line. Used to center various widget heights vertically. This must be non-zero, otherwise the widgets will align wrongly.
	f32 minScrollViewHandleSize = 20.0f; /// the minimum allowed scroll handle size (height)
	bool allowUndockingToNewWindow = true; /// allow pane tabs to be undocked as native windows, outside of main window
	u32 widgetLoopStartId = 1000000000; /// when pushing loops into loop stack, the widget ids will start from here. Basically this avoids the user to specify IDs when creating widgets in a loop, taking into account the fact there will not be so many widgets created anyway.
	u32 widgetLoopMaxCount = 500000; /// current increment after each loop push to stack
};

//////////////////////////////////////////////////////////////////////////
// Core
//////////////////////////////////////////////////////////////////////////

/// Create a new HorusUI context
/// \param customInputProvider a custom input provider which will handle input and windowing
/// \param customGfxProvider a custom graphics provider which will handle rendering of the UI
/// \return the created context handle
HORUS_API HContext createContext(struct ContextSettings& settings);

/// Set the current context
/// \param ctx the context
HORUS_API void setContext(HContext ctx);

/// \return the current context
HORUS_API HContext getContext();

/// Delete a context
/// \param ctx the context to be deleted
HORUS_API void deleteContext(HContext ctx);

/// \return the context settings
HORUS_API ContextSettings& getContextSettings();

/// Gather and process the input events, including window events, called in a main loop
HORUS_API void processInputEvents();

/// Set the current frame time delta. Used for tooltips and other timed things.
/// Must be called continuously in the main loop. If initializeWithSDL is used, no need to call it, the SDL input provider will update it.
/// \param dt delta time value, in seconds
HORUS_API void setFrameDeltaTime(f32 dt);

/// \return delta time in seconds
HORUS_API f32 getFrameDeltaTime();

/// Begin a frame which means the rendering of UI across one or many windows. This must be called first when rendering UI
HORUS_API void beginFrame();

/// Ends an UI frame
HORUS_API void endFrame();

/// Clear the current window background with the color found in the current theme
HORUS_API void clearBackground();

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
HORUS_API u32 getInputEventCount();

/// \return the input event at the index
/// \param index the event index (maximum is getInputEventCount())
HORUS_API InputEvent getInputEventAt(u32 index);

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
// Windowing
//////////////////////////////////////////////////////////////////////////

/// Set the current window
/// \param window the window to be set as current
HORUS_API void setWindow(HWindow window);

/// \return the window set as current
HORUS_API HWindow getWindow();

/// \return the focused window
HORUS_API HWindow getFocusedWindow();

/// \return the mouse hovered window
HORUS_API HWindow getHoveredWindow();

/// \return the application's main window
HORUS_API HWindow getMainWindow();

/// Create a new OS native window
/// \param title the title of the window
/// \param width the width of the window or -1 to use default
/// \param height the height of the window or -1 to use default
/// \param flags the flags of the window
/// \param positionType the position of the window
/// \param customPosition if the position is custom, then this is the location on screen
/// \return the created window handle
HORUS_API HWindow createWindow(
	const char* title, u32 width, u32 height,
	WindowFlags flags = WindowFlags::Resizable | WindowFlags::Centered,
	Point customPosition = { 0, 0 });

/// Set window title
/// \param title the window title
HORUS_API void setWindowTitle(HWindow window, const char* title);

/// Set window rectangle on screen
/// \param window the window handle
/// \param rect the screen rectangle
HORUS_API void setWindowRect(HWindow window, const Rect& rect);

/// \return the window rectangle on screen
/// \param window the window handle
HORUS_API Rect getWindowRect(HWindow window);

/// \return the window client rectangle area, relative to window screen rectangle
/// \param window the window handle
HORUS_API Rect getWindowClientRect(HWindow window);

/// Present the contents of the backbuffer, called after all rendering is done
/// \param window the backbuffer's window to show
HORUS_API void presentWindow(HWindow window);

/// Destroy an OS native window
/// \param window the window to destroy
HORUS_API void destroyWindow(HWindow window);

/// Show a window
/// \param window the window to be shown
HORUS_API void showWindow(HWindow window);

/// Hide a window
/// \param window window to be hidden
HORUS_API void hideWindow(HWindow window);

/// Bring a window to front
/// \param window to be brought to front
HORUS_API void riseWindow(HWindow window);

/// Maximize a window
/// \param window the window to be maximized
HORUS_API void maximizeWindow(HWindow window);

/// Minimize a window
/// \param window the window to be minimized
HORUS_API void minimizeWindow(HWindow window);

/// \return the window's state
/// \param window the window
HORUS_API WindowState getWindowState(HWindow window);

/// Capture input to a specific window
/// \param window the window to capture input events
HORUS_API void setCapture(HWindow window);

/// Release the capture for the input events
HORUS_API void releaseCapture();

/// \return true if the application must quit, due to quitApplication() call.
HORUS_API bool mustQuit();

/// \return true if the application must quit, due to user closing main window. You can close the application by exiting the main loop or ignore it, as you wish. If you show a message box and it is cancelled you must call cancelQuitApplication() to set this returned value to false.
HORUS_API bool wantsToQuit();

/// Cancel quitting the application if it was due to exit
HORUS_API void cancelQuitApplication();

/// Set the quit application to true, so the main loop will end
HORUS_API void quitApplication();

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

HORUS_API void setDefaultWidgetStyle(WidgetType widgetType);

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
// Layout and containers
//////////////////////////////////////////////////////////////////////////

/// Start to create UI inside a specific native window, set it as current window
/// \param window the window
HORUS_API void beginWindow(HWindow window);

/// Stop creating UI in the current window
HORUS_API void endWindow();

/// Begin a widget container, and invisible rectangle on the current window area where widgets will be laid out
HORUS_API void beginContainer(const Rect& rect);

/// End the current widget container
HORUS_API void endContainer();

/// Push a widget loop, used when you create widgets inside a loop.
/// For each pushed loop, the widget IDs will be created incrementally in the upper range of uint32
/// \param loopMaxCount optional, this should be a constant for this specific loop, the max number of widgets that might be in this loop. If -1, use the current loop size set with getSettings().widgetLoopMaxCount
HORUS_API void pushWidgetLoop(u32 loopMaxCount = ~0);

/// Pop a widget id from ID stack, used with pushWidgetId
HORUS_API void popWidgetLoop();

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
HORUS_API void beginEqualColumns(u32 columnCount, const f32 minWidths[] = nullptr, bool addPadding = false);

/// Start a single padded column layout, which has padding added to left and right sides
HORUS_API void beginPaddedColumn();

/// End a single padded column layout
HORUS_API void endPaddedColumn();

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
HORUS_API void pushPadding(f32 newPadding);

/// Pop the previous padding value from stack and set it as current
HORUS_API void popPadding();

/// Push the old spacing value to stack and set a new spacing value, spacing is the vertical space between widgets
/// \param newSpacing the new vertical spacing value
HORUS_API void pushSpacing(f32 newSpacing);

/// Pop old spacing value from stack and set it as current
HORUS_API void popSpacing();

/// \return the current vertical spacing value
HORUS_API f32 getSpacing();

/// \return the current horizontal left and right side padding value
HORUS_API f32 getPadding();

/// Set the global UI scale, this will scale all the elements from widgets to text, but not the docking panes
/// \param scale a value between 0 and N, no higher limit, but use with consideration
HORUS_API void setGlobalScale(f32 scale);

/// \return the current global UI scale
HORUS_API f32 getGlobalScale();

/// Push and set a new tinting color on stack, to colorize the next widget on specific parts
/// \param color the tint color
/// \param type what elements of the widget to tint
HORUS_API void pushTint(const Color& color, TintColorType type = TintColorType::All);

/// Pop the old tint color from stack
HORUS_API void popTint(TintColorType type = TintColorType::All);

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

/// Begin drawing a popup widget on top of all other popups or widgets
/// \param width the width of the popup
/// \param flags the popup flags
/// \param position when custom position, this is the window coordinates of the popup
/// \param widgetElementId will use this element's theme to draw the popup body
HORUS_API void beginPopup(
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
/// \param icon the icon of the message box
/// \param width the width of the message box
/// \param customIcon the custom icon, if set in the icon param
/// \return the pushed button in the message box
HORUS_API MessageBoxButtons messageBox(
	const char* title,
	const char* message,
	MessageBoxButtons buttons = MessageBoxButtons::Ok,
	MessageBoxIcon icon = MessageBoxIcon::Info,
	u32 width = 400,
	HImage customIcon = 0);

/// Set the next widget as enabled or not
/// \param enabled if true, the widget is enabled for input
HORUS_API void setEnabled(bool enabled);

/// Set next widget as focused
HORUS_API void setFocused();

/// Draw a button widget
/// \param labelText the button text
/// \return true if button was pressed
HORUS_API bool button(const char* labelText);

/// Draw a button with an icon on it
/// \param icon the icon image
/// \param height the button height, if zero then it takes the icon's height
/// \param down if true the button is in the pressed state
/// \return true if the button was pressed
HORUS_API bool iconButton(HImage icon, f32 height = 0.0f, bool down = false);

/// Draw a text input widget
/// \param text the text to be edited, provided by user
/// \param maxTextSize the max size of the text buffer
/// \param valueType the value type filter, what value is allowed in the text
/// \param defaultText the grayed default text when there is no text value
/// \param icon the icon drawn in the widget
/// \return true if the text was modified
HORUS_API bool textInput(char* text, u32 maxTextSize, TextInputValueMode valueType = TextInputValueMode::Any, const char* defaultText = nullptr, HImage icon = 0, bool password = false, const char* passwordChar = "•");

/// Draw an integer number slider widget
/// \param minVal the minimum value
/// \param maxVal the maximum value
/// \param value the value ref
/// \param useStep use stepping when moving slider
/// \param step if useStep is true, then this is the step size
/// \return true if value was modified
HORUS_API bool sliderInteger(i32 minVal, i32 maxVal, i32& value, bool useStep = false, i32 step = 0);

/// Draw a float number slider widget
/// \param minVal the minimum value
/// \param maxVal the maximum value
/// \param value the value ref
/// \param useStep use stepping when moving slider
/// \param step if useStep is true, then this is the step size
/// \return true if value was modified
HORUS_API bool sliderFloat(f32 minVal, f32 maxVal, f32& value, bool useStep = false, f32 step = 0);

HORUS_API bool comboSliderFloat(f32& value, f32 stepsPerPixel = 1.0f, f32 arrowStep = 1.0f);
HORUS_API bool comboSliderFloatRanged(f32& value, f32 minVal, f32 maxVal, f32 stepsPerPixel = 1.0f, f32 arrowStep = 1.0f);
HORUS_API bool rotarySliderFloat(const char* label, f32& value, f32 minVal, f32 maxVal, f32 step, bool twoSide = false, f32 fineStepDivideFactor = 10.f);

/// Draw a image widget
/// \param image the image to draw
/// \param height the height of the image, if zero then the actual image height will be used
/// \param horizontalAlign the horizontal image align mode
/// \param verticalAlign the vertical image align mode
/// \param fit how the image is fitted in the rectangle, resize mode
/// \return true if it was clicked on
HORUS_API bool image(HImage image, f32 height = 0, HAlignType horizontalAlign = HAlignType::Center, VAlignType verticalAlign = VAlignType::Center, ImageFitType fit = ImageFitType::KeepAspect);

/// Draw a progress bar widget
/// \param value the progress as a percentage from 0.0f to 1.0f (meaning 100%)
HORUS_API void progress(f32 value);

/// Draw a check box widget
/// \param labelText the label text
/// \param checked true if it has check mark on
/// \return true if it is checked
HORUS_API bool check(const char* labelText, bool checked);

/// Draw a radio box widget
/// \param labelText the label text
/// \param checked true if it has check mark on
/// \return true if it is checked
HORUS_API bool radio(const char* labelText, bool checked);

/// Draw a label text widget
/// \param labelText the label's text
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HORUS_API bool label(const char* labelText, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a label text widget with a custom font
/// \param labelText the label's text
/// \param font the label's font
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HORUS_API bool labelCustomFont(const char* labelText, HFont font, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a multiline label text widget (involves more logic than a single lined label)
/// \param labelText the label's text
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HORUS_API bool multilineLabel(const char* labelText, HAlignType horizontalAlign);

/// Draw a multiline label text widget with a custom font (involves more logic than a single lined label)
/// \param labelText the label's text
/// \param font the label's font
/// \param horizontalAlign the text align mode horizontally in the current layout rectangle
/// \return true if it was clicked on
HORUS_API bool multilineLabelCustomFont(const char* labelText, HFont font, HAlignType horizontalAlign = HAlignType::Left);

/// Draw a expandable panel widget
/// \param labelText the text of the panel
/// \param expanded true if the panel is expanded
/// \return true if the panel is expanded, use this in a if() statement to draw child widgets if expanded
HORUS_API bool panel(const char* labelText, bool expanded);

/// Draw a dropdown widget
/// \param selectedIndex the current selected item index
/// \param items an array of strings for the items
/// \param itemCount the number of items in the list
/// \param maxVisibleDropDownItems the maximum number of visible items in the drop down list, if ~0 then its automatic
/// \return true if it the selection changed
HORUS_API bool dropdown(i32& selectedIndex, const char** items, u32 itemCount, u32 maxVisibleDropDownItems = ~0);

/// Draw a custom data dropdown widget, good for many items in the list
/// \param selectedIndex the current selected item index
/// \param userdata the items user custom data
/// \param itemSource a callback to use when rendering an item, returns false when item list ended
/// \param maxVisibleDropDownItems the maximum number of visible items in the drop down list, if ~0 then its automatic
/// \return true if it the selection changed
HORUS_API bool dropdown(i32& selectedIndex, void* userdata, bool(*itemSource)(void* userdata, i32 index, char** outItemText), u32 maxVisibleDropDownItems = ~0);

/// TODO:
HORUS_API bool list(i32* selectedIndices, u32 maxSelectedIndices, ListSelectionMode selectionType, const char** items, u32 itemCount);

/// TODO: 
HORUS_API bool list(i32* selectedIndices, u32 maxSelectedIndices, ListSelectionMode selectionType, void* userdata, bool(*itemSource)(void* userdata, i32 index, char** outItemText));

/// TODO: 
HORUS_API bool beginList(ListSelectionMode selectionType);

/// TODO: 
HORUS_API void endList();

/// TODO: 
HORUS_API void listItem(const char* labelText, SelectableFlags stateFlags, HImage icon);

/// Draw a selectable label
/// \param labelText the label's text
/// \param stateFlags the state of the selectable widget
/// \return true if it is selected
HORUS_API bool selectable(const char* labelText, SelectableFlags stateFlags = SelectableFlags::Normal);

/// Draw a selectable label with custom font
/// \param labelText the label's text
/// \param font the label's text font
/// \param stateFlags the state of the selectable widget
/// \return true if it is selected
HORUS_API bool selectableCustomFont(const char* labelText, HFont font, SelectableFlags stateFlags = SelectableFlags::Normal);

//////////////////////////////////////////////////////////////////////////
// Separators
//////////////////////////////////////////////////////////////////////////

/// Draw a horizontal line widget
HORUS_API void line();

/// Leave a space between previous widget and next one
/// \param size the size of the gap
HORUS_API void gap(f32 size);

/// Leave a normal space between previous widget and next one
HORUS_API void space();

/// Make next widget show on the same row as the last widget. The widget width depends on the widget type, the content inside it, etc.
/// Not all widgets support the same line modifier, since some need content
HORUS_API void beginSameLine();

HORUS_API void endSameLine();

HORUS_API void pushSameLineSpacing(f32 horizontalSpace = 0.0f);

HORUS_API f32 popSameLineSpacing();

HORUS_API void pushWidth(f32 width);

HORUS_API f32 popWidth();

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
HORUS_API void beginMenuBar();

/// End the current menu bar widget
HORUS_API void endMenuBar();

/// Begin a menu panel widget (it will show up only when clicked)
/// \param labelText the menu item text
/// \param flags the menu item flags
/// \return true if the menu is visible, use it in a if() statement to show menu items
HORUS_API bool beginMenu(const char* labelText, SelectableFlags flags = SelectableFlags::Normal);

/// End the current menu
HORUS_API void endMenu();

/// Begin drawing a context menu which will open on right click on the previous widget
/// \return true if the menu is opened/visible
HORUS_API bool beginContextMenu(ContextMenuFlags flags = ContextMenuFlags::None);

/// End the current context menu
HORUS_API void endContextMenu();

/// Draw a menu item widget, use inside begin/end menu (or context menu)
/// \param labelText the menu item text
/// \param shortcut the key shortcut text
/// \param icon the menu item left side icon
/// \param flags the menu item flags
/// \return true if the menu item was clicked on
HORUS_API bool menuItem(const char* labelText, const char* shortcut, HImage icon = 0, SelectableFlags flags = SelectableFlags::Normal);

/// Draw a menu item separator
HORUS_API void menuSeparator();

//////////////////////////////////////////////////////////////////////////
// Toolbar
//////////////////////////////////////////////////////////////////////////

/// Begin drawing a toolbar widget
/// \param direction the toolbar direction
HORUS_API void beginToolbar(ToolbarDirection dir = ToolbarDirection::Horizontal);

/// End the current toolbar
HORUS_API void endToolbar();

/// Draw a toolbar button widget
/// \param normalIcon the normal state icon
/// \param disabledIcon the disabled state icon
/// \param down true if the button state is down
/// \return true if the button was pressed
HORUS_API bool toolbarButton(HImage normalIcon, HImage disabledIcon = 0, bool down = false);

/// Draw a toolbar dropdown button widget
/// \param label the label text
/// \param normalIcon the normal state icon
/// \param disabledIcon the disabled state icon
/// \param down true if the button state is down
/// \return true if the dropdown button was pressed
HORUS_API bool toolbarDropdown(const char* label, HImage normalIcon = 0, HImage disabledIcon = 0);

/// Draw a toolbar item separator
HORUS_API void toolbarSeparator();

/// Leave a gap horizontally in the toolbar
HORUS_API void toolbarGap(f32 gapSize = 5);

/// Draw a text input filter editor in the toolbar
/// \param outText the text buffer to edit
/// \param maxOutTextSize the maximum size of the buffer
/// \param filterIndex the current filter index
/// \param filterNames the filter names
/// \param filterNameCount the filter name count
/// \return true if the text or filter changed
HORUS_API bool toolbarTextInputFilter(char* outText, u32 maxOutTextSize, u32& filterIndex, const char** filterNames = 0, u32 filterNameCount = 0);

/// Draw a text input widget in the toolbar
/// \param outText the text buffer to edit
/// \param maxOutTextSize the maximum size of the buffer
/// \param hint the text hint
/// \param icon the text edit icon
/// \return true if the text was changed
HORUS_API bool toolbarTextInput(char* outText, u32 maxOutTextSize, const char* hint = 0, HImage icon = 0);

//////////////////////////////////////////////////////////////////////////
// Dockable Tabs
//////////////////////////////////////////////////////////////////////////

/// Start a tab group
/// \param selectedIndex the selected tab index
HORUS_API void beginTabGroup(TabIndex selectedIndex);

/// Draw a tab widget
/// \param labelText the text of the tab
/// \param icon the icon of the tab
HORUS_API void tab(const char* labelText, HImage icon);

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
HORUS_API u32 getWidgetId();

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
HORUS_API Rect beginCustomWidget(f32 height = 0.0f);

/// End custom widget drawing
HORUS_API void endCustomWidget();

/// \return the current widget drawing pen position in the parent layout/container
HORUS_API Point getPenPosition();

/// Set the widget pen position
HORUS_API void setPenPosition(const Point& penPosition);

/// Increment the widget layer index, the highest layer index will be the active one
HORUS_API void incrementLayerIndex();

/// \return the layer index, after decrementing it
HORUS_API u32 decrementLayerIndex();

/// 
HORUS_API void decrementWindowMaxLayerIndex();

/// 
HORUS_API Point getParentSize();

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
// Pane container functions
//////////////////////////////////////////////////////////////////////////

/// Create a view container for a specific window, where views can be docked, used by the docking system
HORUS_API HViewContainer createViewContainer(HWindow window);

/// Delete a view container, used by the docking system
HORUS_API void deleteViewContainer(HViewContainer viewContainer);

/// Get the view containers from all windows, used by the docking system
HORUS_API u32 getViewContainers(HViewContainer* outViewContainers, u32 maxCount);

/// \return the view container's window, used by the docking system
HORUS_API HWindow getViewContainerWindow(HViewContainer viewContainer);

/// \return a view container for a specific window, used by the docking system
HORUS_API HViewContainer getWindowViewContainer(HWindow window);

/// Delete window's view container, used by the docking system
HORUS_API void deleteViewContainerFromWindow(HWindow window);

/// Get view container's view panes, used by the docking system
/// \return -1 if maxCount was too small
HORUS_API u32 getViewContainerViewPanes(HViewContainer viewContainer, HViewPane* outViewPanes, u32 maxCount);

HORUS_API u32 getViewContainerViewPaneCount(HViewContainer viewContainer);

/// Get view container's first view pane, used by the docking system
HORUS_API HViewPane getViewContainerFirstViewPane(HViewContainer viewContainer);

/// Save the view container state, with all view panes docked info
/// \param filename the *.hui filename relative to executable where to save the state
/// \return true if save was ok
HORUS_API bool saveViewContainersState(const char* filename);

/// Load the view container state, with all view panes docked info, it will create view panes
/// \param filename the *.hui filename relative to executable from where to load the state
/// \return true if the load was ok
HORUS_API bool loadViewContainersState(const char* filename);

/// Set the view container spacing for adding toolbars, status bar or panels
/// Top spacing is not needed, it will be computed automatically from the rendered widgets heights
HORUS_API void setViewContainerSideSpacing(HViewContainer viewContainer, f32 left, f32 right, f32 bottom);

//////////////////////////////////////////////////////////////////////////
// Pane and tab functions
//////////////////////////////////////////////////////////////////////////

/// \return the view pane rect
/// \param viewPane the view pane
HORUS_API Rect getViewPaneRect(HViewPane viewPane);

/// \return the remaining view pane height
/// \param viewPane the view pane
HORUS_API f32 getRemainingViewPaneHeight(HViewPane viewPane);

/// Create a new view pane
/// \param viewContainer the parent view container
/// \param dockType how the view pane will dock
/// \param paneSize width or height, depending on dock type
/// \return the view pane handle
HORUS_API HViewPane createViewPane(HViewContainer viewContainer, DockType dockType, f32 paneSize = 0.0f);

/// Create a new view pane as a child of another view pane
/// \param parentViewPane the parent view pane
/// \param dockType how the view pane will dock
/// \param paneSize width or height, depending on dock type
/// \return the view pane handle
HORUS_API HViewPane createChildViewPane(HViewPane parentViewPane, DockType dockType, f32 paneSize = 0.0f);

/// Add a tab (view instance) to a view pane tab group
/// \param viewPane the view pane where to add a tab
/// \param title the tab text
/// \param id the view id to be shown in this tab
/// \param userDataId the data id associated with this tab's view
/// \return a view pane tab handle
HORUS_API HViewPaneTab addViewPaneTab(HViewPane viewPane, const char* title, ViewId id, u64 userDataId);

/// Remove a view pane tab
HORUS_API void removeViewPaneTab(HViewPaneTab viewPaneTab);

/// Set a view pane tab data id
HORUS_API void setViewPaneTabUserDataId(HViewPaneTab viewPaneTab, u64 userDataId);

/// \return a tab view's data id
HORUS_API u64 getViewPaneTabUserDataId(HViewPaneTab viewPaneTab);

HORUS_API void setViewPaneTabTitle(HViewPaneTab viewPaneTab, const char* title);

HORUS_API const char* getViewPaneTabTitle(HViewPaneTab viewPaneTab);

HORUS_API ViewId getViewPaneTabViewId(HViewPaneTab viewPaneTab);

/// Set a view icon
HORUS_API void setViewIcon(ViewId id, HImage image);

/// Dock a view pane inside a view container
HORUS_API void dockViewPane(HViewPane viewPane, HViewContainer viewContainer, DockType dockType);

/// Begin draw of a view pane
HORUS_API ViewId beginViewPane(HViewPane viewPane);

/// End draw of a view pane
HORUS_API void endViewPane();

/// Activate a view pane
HORUS_API void activateViewPane(HViewPane viewPane);

/// Close a view pane
HORUS_API void closeViewPane(HViewPane viewPane);

/// Maximize a view pane
HORUS_API void maximizeViewPane(HViewPane viewPane);

/// Restore a view pane
HORUS_API void restoreViewPane(HViewPane viewPane);

/// Get view pane's view pane tabs
/// \return -1 if maxCount was too small
HORUS_API u32 getViewPaneTabs(HViewPane viewPane, HViewPaneTab* outViewPaneTabs, u32 maxCount);

HORUS_API u32 getViewPaneTabCount(HViewPane viewPane);

//////////////////////////////////////////////////////////////////////////
// Docking system functions
//////////////////////////////////////////////////////////////////////////

/// Set the current view handler, used throughout the docking system (also for save/load view window state)
HORUS_API void setCurrentViewHandler(ViewHandler* handler);

/// \return the current view handler, used throughout the docking system (also for save/load view window state)
HORUS_API ViewHandler* getCurrentViewHandler();

/// Update the docking system internal, usually called by the dockingSystemLoop function, if you make your own loop, then you need to call it
HORUS_API void updateDockingSystem();

/// If this function will be called it will block until all or the main window is closed, or a quitApplication is issued
HORUS_API void dockingSystemLoop();

//////////////////////////////////////////////////////////////////////////
// Utility panels and complex/combined mega-widgets
//////////////////////////////////////////////////////////////////////////

/// Draw a color picker popup widget
HORUS_API bool colorPickerPopup(const Color& currentColor, Color& outNewColor);

/// Draw a 3D double vector editor widget
HORUS_API bool vec3Editor(f64& x, f64& y, f64& z, f64 scrollStep = 0.03f);

/// Draw a 3D float vector editor widget
HORUS_API bool vec3Editor(f32& x, f32& y, f32& z, f32 scrollStep = 0.03f);

/// Draw a 2D double vector editor widget
HORUS_API bool vec2Editor(f64& x, f64& y, f64 scrollStep = 0.03f);

/// Draw a 2D float vector editor widget
HORUS_API bool vec2Editor(f32& x, f32& y, f32 scrollStep = 0.03f);

/// Draw an object reference editor
HORUS_API bool objectRefEditor(HImage targetIcon, HImage clearIcon, const char* objectTypeName, const char* valueAsString, u32 objectType, void** outObject, bool* objectValueWasModified);

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
HORUS_API void toString(i32 value, char* outString, u32 outStringMaxSize, u32 fillerZeroesCount = 0);

/// Convert a float value to string
HORUS_API void toString(f32 value, char* outString, u32 outStringMaxSize, u32 decimalPlaces = 4);

/// Convert a unicode (utf32) value to utf8 string
HORUS_API bool unicodeToUtf8(const u32* text, u32 maxTextSize, char* outString, u32 maxOutStringSize);

HORUS_API bool getColorFromText(const char* colorText, Color& color);

}
/** @}*/