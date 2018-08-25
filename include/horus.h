#pragma once
#include <stdio.h>
#include <stdint.h>
#include <math.h>

/*
------------------------------------------------------------------------------
	Horus UI
------------------------------------------------------------------------------
	Immediate Mode Graphical User Interface Library

	(C) All rights reserved 2016-2017 7thFACTOR Software - Nicusor Nedelcu
------------------------------------------------------------------------------
*/

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
		#ifdef HORUS_EXPORTS
			#define HORUS_API extern "C++" __declspec(dllexport)
			#define HORUS_CLASS_API __declspec(dllexport)
		#else
			#define HORUS_API extern "C++" __declspec(dllimport)
			#define HORUS_CLASS_API __declspec(dllimport)
		#endif
	#else
		#ifdef HORUS_EXPORTS
			#define HORUS_API __attribute__((dllexport))
			#define HORUS_CLASS_API __attribute__((dllexport))
		#else
			#define HORUS_API __attribute__((dllimport))
			#define HORUS_CLASS_API __attribute__((dllimport))
		#endif
	#endif
#endif

namespace hui
{
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

typedef void* Image;
typedef void* Theme;
typedef void* Atlas;
typedef void* Font;
typedef void* ThemeWidgetElement;
typedef void* Window;
typedef void* ViewPane;
typedef void* ViewPaneTab;
typedef void* ViewContainer;
typedef void* MouseCursor;
typedef void* GraphicsApiContext;
typedef void* GraphicsApiTexture;
typedef void* GraphicsApiRenderTarget;
typedef void* GraphicsApiVertexBuffer;
typedef void* Context;
typedef u32 Rgba32;
typedef const char* Utf8String;
typedef char* Utf8StringBuffer;
typedef u32 TabIndex;
typedef u32 ViewId;

const f32 ColumnFill = -1;

enum class HAlignType
{
	Left,
	Right,
	Center
};

enum class VAlignType
{
	Top,
	Bottom,
	Center
};

enum class FontStyle
{
	Normal,
	Bold,
	Italic,
	BoldItalic,
	
	Count
};

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

	Count
};

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
	DropdownBody,
	DropdownArrow,
	ScrollViewBody,
	ScrollViewScrollBar,
	ScrollViewScrollThumb,
	TabGroupBody,
	TabBodyActive,
	TabBodyInactive,
	ViewPaneDockRect,
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
	ToolbarSeparatorBody,
	ColumnsHeaderBody,

	Count
};

enum class WidgetStateType
{
	Normal,
	Focused,
	Pressed,
	Hovered,
	Disabled,

	Count
};

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

enum class ImageFitType
{
	None,
	KeepAspect,
	Stretch
};

enum class TextInputValueMode
{
	Any,
	NumericOnly,
	HexOnly,
	Custom
};

enum class ListSelectionType
{
	Single,
	Multiple
};

enum class SelectableFlags : u32
{
	Normal = (1 << 0),
	Checkable = (1 << 1),
	Checked = (1 << 2),
	Disabled = (1 << 3),
	Selected = (1 << 4)
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

enum class PopupPositionMode
{
	WindowCenter,
	BelowLastWidget,
	Custom
};

enum class LayoutType
{
	Container,
	Vertical,
	Columns,
	Column,
	ScrollView
};

enum class TintColorType
{
	Body,
	Text,
	All,

	Count
};

enum class WindowPositionType
{
	Undefined,
	Centered,
	Custom
};

enum class WindowBorder
{
	Resizable,
	Fixed,
	ResizableNoTitle,
	FixedNoTitle
};

enum class WindowState
{
	Normal,
	Maximized,
	Minimized,
	Hidden
};

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
	Shift = (1 << 0),
	Control = (1 << 1),
	Alt = (1 << 2),
	CapsLock = (1 << 3)
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

enum class DockType
{
	Left,
	Right,
	TopAsViewTab,
	Top,
	Bottom
};

enum class MessageBoxIcon
{
	Error,
	Info,
	Question,
	Warning
};

enum class MessageBoxButtons : u32
{
	None = 0,
	Ok = (1 << 0),
	Cancel = (1 << 1),
	Yes = (1 << 2),
	No = (1 << 3),
	Retry = (1 << 4),
	Abort = (1 << 5),
	ClosedByEscape = (1 << 6),
	OkCancel = (u32)Ok | (u32)Cancel,
	YesNo = (u32)Yes | (u32)No,
	YesNoCancel = (u32)YesNo | (u32)Cancel
};
HORUS_ENUM_AS_FLAGS(MessageBoxButtons);

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

		m = sqrt(m);
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

		m = sqrt((f32)m);
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

		m = sqrt(m);
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
			&& y != other.y;
	}

	inline Point& operator = (const Point& other)
	{
		x = other.x;
		y = other.y;

		return *this;
	}

	f32 x, y;
};

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
	{ return pt.x >= x && pt.x < (x + width) && pt.y >= y && (pt.y < y + height); }
	
	inline bool contains(f32 X, f32 Y) const
	{ return X >= x && X < (x + width) && Y >= y && Y < (y + height); }
	
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
		Window window = 0;
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
	Window window = 0;
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

struct ViewHandler
{
	virtual void onMainMenuRender(Window window) {}
	virtual void onViewPaneRender(Window window, Window viewPane, ViewId activeViewId, u64 userDataId) {}
	virtual void onViewPaneClosed(Window window, Window viewPane, ViewId activeViewId, u64 userDataId) {}
	virtual void onBeforeFrameRender() {}
	virtual void onAfterFrameRender() {}
};

struct LineStyle
{
	LineStyle() {}
	LineStyle(const Color& newColor, f32 newWidth)
		: color(newColor)
		, width(newWidth)
	{}

	Color color = Color::white;
	f32 width = 1.0f;
};

struct FillStyle
{
	FillStyle() {}
	FillStyle(const Color& newColor)
		: color(newColor)
	{}

	Color color = Color::white;
	GraphicsApiTexture texture;
	Point scale;
};

struct RawImage
{
	u8* pixels = nullptr;
	u32 width = 0;
	u32 height = 0;
	u32 bpp = 0;
};

struct WidgetElementInfo
{
	//! the image from the theme, used to draw the element
	Image image;
	//! the border size used to draw 9-cell resizable element
	u32 border;
	//! the color of the element
	Color color;
	//! the text color of the element
	Color textColor;
	//! the font used for this element
	Font font;
	//! the pixel width of the element (not its image)
	f32 width;
	//! the pixel height of the element (not its image)
	f32 height;
};

struct ContextSettings
{
	f32 radioBulletTextSpacing = 5;
	f32 checkBulletTextSpacing = 5;
};

//////////////////////////////////////////////////////////////////////////
// Core
//////////////////////////////////////////////////////////////////////////
HORUS_API Context createContext(struct InputProvider* customInputProvider = nullptr, struct GraphicsProvider* customGfxProvider = nullptr);
HORUS_API void initializeContext(Context ctx);
HORUS_API void setContext(Context ctx);
HORUS_API Context getContext();
HORUS_API void deleteContext(Context ctx);
HORUS_API ContextSettings& getContextSettings();
HORUS_API void setInputProvider(struct InputProvider* provider);
HORUS_API void setGraphicsProvider(struct GraphicsProvider* provider);
HORUS_API void processInputEvents();
// must be called continuously in the main loop, used for tooltips and other timed things
HORUS_API void setFrameDeltaTime(f32 dt);
HORUS_API f32 getFrameDeltaTime();
HORUS_API void update();
HORUS_API void beginFrame();
HORUS_API void endFrame();
HORUS_API void clearBackground();
HORUS_API bool hasNothingToDo();
HORUS_API void setDisableRendering(bool disable);
HORUS_API void forceRepaint();
HORUS_API void skipThisFrame();
HORUS_API bool copyToClipboard(Utf8String text);
HORUS_API bool pasteFromClipboard(Utf8String *outText);
HORUS_API void enableInput(bool enabled);
HORUS_API const InputEvent& getInputEvent();
HORUS_API void cancelEvent();
HORUS_API void addInputEvent(const InputEvent& event);
HORUS_API void setMouseMoved(bool moved);
HORUS_API u32 getInputEventCount();
HORUS_API InputEvent getInputEventAt(u32 index);
HORUS_API void setInputEvent(const InputEvent& event);
HORUS_API void clearInputEventQueue();
HORUS_API void setMouseCursor(MouseCursorType type);
HORUS_API MouseCursor createMouseCursor(Rgba32* pixels, u32 width, u32 height, u32 hotSpotX = 0, u32 hotSpotY = 0);
HORUS_API MouseCursor createMouseCursor(const char* imageFilename, u32 hotSpotX = 0, u32 hotSpotY = 0);
HORUS_API void destroyMouseCursor(MouseCursor cursor);
HORUS_API void setMouseCursor(MouseCursor cursor);

//////////////////////////////////////////////////////////////////////////
// Windowing
//////////////////////////////////////////////////////////////////////////
HORUS_API void setWindow(Window window);
HORUS_API Window getWindow();
HORUS_API Window getFocusedWindow();
HORUS_API Window getHoveredWindow();
HORUS_API Window getMainWindow();
HORUS_API Window createWindow(
	Utf8String title, u32 width, u32 height,
	WindowBorder border = WindowBorder::Resizable,
	WindowPositionType positionType = WindowPositionType::Undefined,
	Point customPosition = { 0, 0 },
	bool showInTaskBar = true);
HORUS_API void setWindowTitle(Window window, Utf8String title);
HORUS_API void setWindowRect(Window window, const Rect& rect);
HORUS_API Rect getWindowRect(Window window);
HORUS_API Rect getWindowClientRect(Window window);
HORUS_API void presentWindow(Window window);
HORUS_API void destroyWindow(Window window);
HORUS_API void showWindow(Window window);
HORUS_API void hideWindow(Window window);
HORUS_API void riseWindow(Window window);
HORUS_API void maximizeWindow(Window window);
HORUS_API void minimizeWindow(Window window);
HORUS_API WindowState getWindowState(Window window);
HORUS_API void setCapture(Window window);
HORUS_API void releaseCapture();
HORUS_API bool mustQuit();
HORUS_API bool wantsToQuit();
HORUS_API void cancelQuitApplication();
HORUS_API void quitApplication();
HORUS_API void shutdown();

//////////////////////////////////////////////////////////////////////////
// Images
//////////////////////////////////////////////////////////////////////////
HORUS_API Image loadImage(const char* filename);
HORUS_API Image createImage(Rgba32* pixels, u32 width, u32 height);
HORUS_API Point getImageSize(Image image);
HORUS_API void updateImagePixels(Image image, Rgba32* pixels);
HORUS_API void deleteImage(Image image);
HORUS_API RawImage loadRawImage(const char* filename);
HORUS_API void deleteRawImage(RawImage& image);

//////////////////////////////////////////////////////////////////////////
// Image atlas
//////////////////////////////////////////////////////////////////////////
HORUS_API Atlas createAtlas(u32 width, u32 height);
HORUS_API void deleteAtlas(Atlas atlas);
HORUS_API Image addAtlasImage(Atlas atlas, const RawImage& image);
HORUS_API bool packAtlas(Atlas atlas);

//////////////////////////////////////////////////////////////////////////
// Themes
//////////////////////////////////////////////////////////////////////////
HORUS_API Theme loadTheme(Utf8String filename);
HORUS_API void setTheme(Theme theme);
HORUS_API Theme getTheme();
HORUS_API void deleteTheme(Theme theme);
HORUS_API Theme createTheme(u32 atlasTextureSize, u32 defaultFontSize, Utf8String defaultFontFilename);
HORUS_API Image addThemeImage(Theme theme, const RawImage& img);
HORUS_API void setThemeWidgetElement(
	Theme theme,
	WidgetElementId elementId,
	WidgetStateType widgetStateType,
	const WidgetElementInfo& elementInfo);
void buildTheme(Theme theme);
HORUS_API void setThemeUserWidgetElement(
	Theme theme,
	const char* userElementName,
	WidgetStateType widgetStateType,
	const WidgetElementInfo& elementInfo);
HORUS_API void getThemeWidgetElementInfo(WidgetElementId elementId, WidgetStateType state, WidgetElementInfo& outInfo);
HORUS_API void getThemeUserWidgetElementInfo(const char* userElementName, WidgetStateType state, WidgetElementInfo& outInfo);
HORUS_API Font createFont(Theme theme, const char* fontFilename, u32 faceSize);
//! release font reference, if font usage is zero, the font is deleted
HORUS_API void releaseFont(Font font);
//! \return the font by name, from the current theme
HORUS_API Font getFont(Utf8String themeFontName);
//! \return the font by name, from the specified theme
HORUS_API Font getFont(Theme theme, Utf8String themeFontName);

//////////////////////////////////////////////////////////////////////////
// Layout and containers
//////////////////////////////////////////////////////////////////////////
HORUS_API void beginWindow(Window window);
HORUS_API void endWindow();
HORUS_API void beginContainer(const Rect& rect);
HORUS_API void endContainer();
HORUS_API void pushWidgetId(u32 id);
HORUS_API void popWidgetId();
//! if width <= 1 its considered percentage of the parent, if > 1 its considered fixed pixel size
HORUS_API void beginColumns(u32 columnCount, const f32 preferredWidths[] = nullptr, const f32 minWidths[] = nullptr, const f32 maxWidths[] = nullptr);
HORUS_API void beginEqualColumns(u32 columnCount, const f32 minWidths[] = nullptr, bool addPadding = false);
HORUS_API void beginPaddedColumn();
HORUS_API void endPaddedColumn();
HORUS_API void beginTwoColumns();
HORUS_API void beginThreeColumns();
HORUS_API void beginFourColumns();
HORUS_API void beginFiveColumns();
HORUS_API void beginSixColumns();
HORUS_API void nextColumn();
HORUS_API Rect getColumnRect();
HORUS_API void endColumns();
HORUS_API void columnHeader(Utf8String label, f32& width, f32 preferredWidth, f32 minWidth, f32 maxWidth);
HORUS_API void beginScrollView(f32 height, f32 scrollPosition, f32 virtualHeight = 0.0f);
HORUS_API f32 endScrollView();
HORUS_API void beginVirtualListContent(u32 totalRowCount, u32 itemHeight, f32 scrollPosition);
HORUS_API void endVirtualListContent();
HORUS_API void pushPadding(f32 newPadding);
HORUS_API void popPadding();
HORUS_API void pushSpacing(f32 newSpacing);
HORUS_API void popSpacing();
HORUS_API f32 getSpacing();
HORUS_API f32 getPadding();
HORUS_API void setGlobalScale(f32 scale);
HORUS_API f32 getGlobalScale();
HORUS_API void pushTint(const Color& color, TintColorType type = TintColorType::All);
HORUS_API void popTint(TintColorType type = TintColorType::All);
HORUS_API bool tooltip(Utf8String text);
HORUS_API bool beginRichTooltip(f32 width);
HORUS_API void endRichTooltip();
HORUS_API void beginBox(
	const Color& color,
	WidgetElementId widgetElementId = WidgetElementId::BoxBody,
	WidgetStateType state = WidgetStateType::Normal,
	f32 customHeight = 0.0f);
HORUS_API void beginBox(
	const Color& color,
	const char* userElementName,
	WidgetStateType state = WidgetStateType::Normal,
	f32 customHeight = 0.0f);
HORUS_API bool endBox();
HORUS_API void beginPopup(
	f32 width,
	PopupPositionMode positionMode = PopupPositionMode::BelowLastWidget,
	const Point& position = Point(),
	WidgetElementId widgetElementId = WidgetElementId::PopupBody,
	bool incrementLayer = true,
	bool topMost = false,
	bool isMenu = false);
HORUS_API void endPopup();
HORUS_API void closePopup();
HORUS_API bool mustClosePopup();
HORUS_API bool clickedOutsidePopup();
HORUS_API bool mouseOutsidePopup();
HORUS_API bool pressedEscapeOnPopup();
HORUS_API MessageBoxButtons messageBox(
	Utf8String title,
	Utf8String message,
	MessageBoxButtons buttons = MessageBoxButtons::Ok,
	MessageBoxIcon icon = MessageBoxIcon::Info,
	u32 width = 400,
	Image customIcon = 0);
HORUS_API void setEnabled(bool enabled);
HORUS_API void setFocused();
HORUS_API bool button(Utf8String labelText);
HORUS_API bool iconButton(Image icon, f32 height = 0.0f, bool down = false);
HORUS_API bool textInput(Utf8String text, size_t maxLength, TextInputValueMode valueType = TextInputValueMode::Any, Utf8String defaultText = nullptr, Image icon = 0);
HORUS_API bool sliderInteger(i32 minVal, i32 maxVal, i32& value, bool useStep = false, i32 step = 0);
HORUS_API bool sliderFloat(f32 minVal, f32 maxVal, f32& value, bool useStep = false, f32 step = 0);
HORUS_API bool image(Image image, f32 height = 0, HAlignType horizontalAlign = HAlignType::Center, VAlignType verticalAlign = VAlignType::Center, ImageFitType fit = ImageFitType::KeepAspect);
HORUS_API void progress(f32 value);
HORUS_API bool check(Utf8String labelText, bool checked);
HORUS_API bool radio(Utf8String labelText, bool checked);
HORUS_API bool label(Utf8String labelText, HAlignType horizontalAlign = HAlignType::Left);
HORUS_API bool labelCustomFont(Utf8String labelText, Font font, HAlignType horizontalAlign = HAlignType::Left);
HORUS_API bool multilineLabel(Utf8String labelText, HAlignType horizontalAlign);
HORUS_API bool multilineLabelCustomFont(Utf8String labelText, Font font, HAlignType horizontalAlign = HAlignType::Left);
HORUS_API bool panel(Utf8String labelText, bool expanded);
HORUS_API bool dropdown(i32& selectedIndex, Utf8String* items, u32 itemCount, u32 maxVisibleDropDownItems = ~0);
HORUS_API bool dropdown(i32& selectedIndex, void* userdata, bool (*itemSource)(void* userdata, i32 index, Utf8String* outItem), u32 maxVisibleDropDownItems = ~0);
//TODO:
HORUS_API i32 list(i32 selectedIndex, ListSelectionType selectionType, Utf8String** items, u32 itemCount);
HORUS_API i32 list(i32 selectedIndex, ListSelectionType selectionType, Utf8String* items);
HORUS_API i32 list(i32 selectedIndex, ListSelectionType selectionType, void* userdata, bool(*itemSource)(void* userdata, i32 index, Utf8String** outItem));
HORUS_API i32 beginList(ListSelectionType selectionType);
HORUS_API void endList();
HORUS_API void listItem(Utf8String labelText, SelectableFlags stateFlags, Image icon);
HORUS_API bool selectable(Utf8String labelText, SelectableFlags stateFlags = SelectableFlags::Normal);
HORUS_API bool selectableCustomFont(Utf8String labelText, Font font, SelectableFlags stateFlags = SelectableFlags::Normal);

//////////////////////////////////////////////////////////////////////////
// Separators
//////////////////////////////////////////////////////////////////////////
HORUS_API void line();
HORUS_API void gap(f32 size);
HORUS_API void space();
HORUS_API Rect beginViewport(f32 height = 0);
HORUS_API void endViewport();

//////////////////////////////////////////////////////////////////////////
// Menus
//////////////////////////////////////////////////////////////////////////
HORUS_API void beginMenuBar();
HORUS_API void endMenuBar();
HORUS_API bool beginMenu(Utf8String labelText, SelectableFlags flags = SelectableFlags::Normal);
HORUS_API void endMenu();
HORUS_API bool beginContextMenu();
HORUS_API void endContextMenu();
HORUS_API bool menuItem(Utf8String labelText, Utf8String shortcut, Image icon = 0, SelectableFlags flags = SelectableFlags::Normal);
HORUS_API void menuSeparator();

//////////////////////////////////////////////////////////////////////////
// Toolbar
//////////////////////////////////////////////////////////////////////////
HORUS_API void beginToolbar(u32 itemCount, const f32 preferredWidths[] = nullptr, const f32 minWidths[] = nullptr, const f32 maxWidths[] = nullptr);
HORUS_API void endToolbar();
HORUS_API bool toolbarButton(Image normalIcon, Image disabledIcon = 0, bool down = false);
HORUS_API bool toolbarDropdown(Utf8String label, Image normalIcon = 0, Image disabledIcon = 0);
HORUS_API void toolbarSeparator();
HORUS_API void toolbarGap();
HORUS_API bool toolbarTextInputFilter(char* outText, u32 maxOutTextSize, u32& filterIndex, Utf8String* filterNames = 0, u32 filterNameCount = 0);
HORUS_API bool toolbarTextInput(char* outText, u32 maxOutTextSize, Utf8String hint = 0, Image icon = 0);

//////////////////////////////////////////////////////////////////////////
// Dockable Tabs
//////////////////////////////////////////////////////////////////////////
HORUS_API void beginTabGroup(TabIndex selectedIndex);
HORUS_API void tab(Utf8String labelText, Image icon);
HORUS_API TabIndex endTabGroup();

//////////////////////////////////////////////////////////////////////////
// Immediate state query for the last widget
//////////////////////////////////////////////////////////////////////////
HORUS_API bool isHovered();
HORUS_API bool isFocused();
HORUS_API bool isPressed();
HORUS_API bool isClicked();
HORUS_API bool isVisible();
HORUS_API u32 getWidgetId();
HORUS_API Point getMousePosition();

//////////////////////////////////////////////////////////////////////////
// Drag and drop logic support
//////////////////////////////////////////////////////////////////////////
HORUS_API bool wantsToDragDrop();
HORUS_API void setDragDropMouseCursor(MouseCursor dropAllowedCursor);
HORUS_API void beginDragDrop(u32 dragObjectUserType, void* dragObject);
HORUS_API void endDragDrop();
HORUS_API void allowDragDrop();
HORUS_API void disallowDragDrop();
HORUS_API bool droppedOnWidget();
HORUS_API void* getDragDropObject();
HORUS_API u32 getDragDropObjectType();

//////////////////////////////////////////////////////////////////////////
// Custom widgets
//////////////////////////////////////////////////////////////////////////
HORUS_API Rect beginCustomWidget(f32 height = 0.0f);
HORUS_API void endCustomWidget();
HORUS_API Point getPenPosition();
HORUS_API void setPenPosition(const Point& penPosition);
// the highest layer will be the active one
HORUS_API void incrementLayerIndex();
HORUS_API u32 decrementLayerIndex();
HORUS_API void decrementWindowMaxLayerIndex();
HORUS_API Point getParentSize();
HORUS_API Rect getWidgetRect();
HORUS_API void pushDrawCommandIndex();
HORUS_API u32 popDrawCommandIndex();
HORUS_API void beginInsertDrawCommands(u32 atIndex);
HORUS_API void endInsertDrawCommands();
HORUS_API void setFont(Font font);
HORUS_API void setBackColor(const Color& color);
HORUS_API void drawTextAt(Utf8String text, const Point& position);
HORUS_API void drawTextInBox(Utf8String text, const Rect& rect, HAlignType horizontalAlign, VAlignType verticalAlign);
HORUS_API Point getTextSize(Utf8String text);
HORUS_API void drawImage(Image image, const Point& position);
HORUS_API void drawStretchedImage(Image image, const Rect& rect);
HORUS_API void drawBorderedImage(Image image, u32 border, const Rect& rect);
HORUS_API void setLineStyle(const LineStyle& style);
HORUS_API void setFillStyle(const FillStyle& style);
HORUS_API void drawLine(const Point& a, const Point& b);
HORUS_API void drawPolyLine(const Point* points, u32 pointCount, bool closed = false);
HORUS_API void drawCircle(const Point& center, f32 radius, u32 segments = 32);
HORUS_API void drawEllipse(const Point& center, f32 radiusX, f32 radiusY, u32 segments = 32);
HORUS_API void drawRectangle(const Rect& rc);
HORUS_API void drawSpline(SplineControlPoint* points, u32 count);
HORUS_API void drawArrow(const Point& startPoint, const Point& endPoint, f32 tipLength, f32 tipWidth, bool drawBodyLine = true);

//////////////////////////////////////////////////////////////////////////
// Pane container functions
//////////////////////////////////////////////////////////////////////////
HORUS_API ViewContainer createViewContainer(Window window);
HORUS_API void deleteViewContainer(ViewContainer viewContainer);
HORUS_API size_t getViewContainers(ViewContainer* outViewContainers, size_t maxCount);
HORUS_API Window getViewContainerWindow(ViewContainer viewContainer);
HORUS_API ViewContainer getWindowViewContainer(Window window);
HORUS_API void deleteViewContainerFromWindow(Window window);
// \return -1 if maxCount was too small
HORUS_API size_t getViewContainerViewPanes(ViewContainer viewContainer, ViewPane* outViewPanes, size_t maxCount);
HORUS_API ViewPane getViewContainerFirstViewPane(ViewContainer viewContainer);
HORUS_API bool saveViewContainersState(Utf8String filename);
HORUS_API bool loadViewContainersState(Utf8String filename);

//////////////////////////////////////////////////////////////////////////
// Pane and tab functions
//////////////////////////////////////////////////////////////////////////
HORUS_API Rect getViewPaneRect(ViewPane viewPane);
HORUS_API f32 getRemainingViewPaneHeight(ViewPane viewPane);
HORUS_API ViewPane createViewPane(ViewContainer viewContainer, DockType dockType, f32 paneSize = 0.0f);
HORUS_API ViewPane createChildViewPane(ViewPane parentViewPane, DockType dockType, f32 paneSize = 0.0f);
HORUS_API ViewPaneTab addViewPaneTab(ViewPane viewPane, Utf8String title, ViewId id, u64 userDataId);
HORUS_API void removeViewPaneTab(ViewPaneTab viewPaneTab);
HORUS_API void setTabUserDataId(ViewPaneTab viewPaneTab, u64 userDataId);
HORUS_API u64 getTabUserDataId(ViewPaneTab viewPaneTab);
HORUS_API void setViewIcon(ViewId id, Image image);
HORUS_API void dockViewPane(ViewPane viewPane, ViewContainer viewContainer, DockType dockType);
HORUS_API ViewId beginViewPane(ViewPane viewPane);
HORUS_API void endViewPane();
HORUS_API void activateViewPane(ViewPane viewPane);
HORUS_API void closeViewPane(ViewPane viewPane);
HORUS_API void maximizeViewPane(ViewPane viewPane);
HORUS_API void restoreViewPane(ViewPane viewPane);

//////////////////////////////////////////////////////////////////////////
// Docking system functions
//////////////////////////////////////////////////////////////////////////
HORUS_API void updateDockingSystem(ViewHandler* handler);
HORUS_API void dockingSystemLoop(ViewHandler* handler);
HORUS_API void setAllowUndockingToNewWindow(bool allow);

//////////////////////////////////////////////////////////////////////////
// Utility panels and complex/combined mega-widgets
//////////////////////////////////////////////////////////////////////////
HORUS_API bool colorPickerPopup(const Color& currentColor, Color& outNewColor);
HORUS_API bool vec3Editor(f64& x, f64& y, f64& z, f64 scrollStep = 0.03f);
HORUS_API bool vec3Editor(f32& x, f32& y, f32& z, f32 scrollStep = 0.03f);
HORUS_API bool vec2Editor(f64& x, f64& y, f64 scrollStep = 0.03f);
HORUS_API bool vec2Editor(f32& x, f32& y, f32 scrollStep = 0.03f);
HORUS_API bool objectRefEditor(Image targetIcon, Image clearIcon, Utf8String objectTypeName, Utf8String valueAsString, u32 objectType, void** outObject, bool* objectValueWasModified);

//////////////////////////////////////////////////////////////////////////
// System native file dialogs
//////////////////////////////////////////////////////////////////////////
typedef struct
{
	char* filenameBuffer = nullptr;
	size_t* bufferIndices = nullptr;
	size_t count = 0;
} OpenMultipleFileSet;

HORUS_API bool openFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 outPathMaxSize);
HORUS_API bool openMultipleFileDialog(const char* filterList, const char* defaultPath, OpenMultipleFileSet& outPathSet);
HORUS_API void destroyMultipleFileSet(OpenMultipleFileSet& fileSet);
HORUS_API bool saveFileDialog(const char* filterList, const char* defaultPath, char* outPath, u32 outPathMaxSize);
HORUS_API bool pickFolderDialog(const char* defaultPath, char* outPath, u32 outPathMaxSize);

//////////////////////////////////////////////////////////////////////////
// Utility functions
//////////////////////////////////////////////////////////////////////////
HORUS_API void toString(i32 value, char* outString, u32 outStringMaxSize, u32 fillerZeroesCount = 0);
HORUS_API void toString(f32 value, char* outString, u32 outStringMaxSize, u32 decimalPlaces = 4);
HORUS_API bool unicodeToUtf8(const u32* text, size_t textLength, Utf8StringBuffer outString, size_t maxOutStringLength);

}