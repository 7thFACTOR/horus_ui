#pragma once
#include "horus.h"
#include <string>
#include <vector>

namespace hui
{
enum class RenderMode
{
	ScreenSpace,
	WorldSpace
};

enum class ScaleMode
{
	ConstantPixelSize,
	ScaleWithScreenSize
};

enum class ElementType
{
	None,
	Text,
	Image,
	Button,
	Toggle,
	Slider,
	Scrollbar,
	Dropdown,
	TextInput,
	ScrollRect,

	Count
};

enum class StateType
{
	Normal,
	Hovered,
	Pressed,
	Disabled,

	Count
};

enum class EaseType
{
	InLinear,
	outLinear,
	InOutLinear,

	InBack,
	OutBack,
	InOutBack,

	InBounce,
	OutBounce,
	InOutBounce,

	InCirc,
	OutCirc,
	InOutCirc,

	InCubic,
	OutCubic,
	InOutCubic,

	InElastic,
	OutElastic,
	InOutElastic,

	InExpo,
	OutExpo,
	InOutExpo,

	InQuad,
	OutQuad,
	InOutQuad,

	InQuart,
	OutQuart,
	InOutQuart,

	InQuint,
	OutQuint,
	InOutQuint,

	InSine,
	OutSine,
	InOutSine,

	Count
};

struct Vec3
{
	float x, y, z;

	inline void reset() { x = y = z = 0; }
};

struct Matrix44
{
	f32 m[16];
};

struct InputEvent3D
{
	InputEvent event2D;
	Vec3 rayStart, rayDir;
};

struct Transform
{
	Vec3 position, pivot, scale, rotation;
	Point size;
	Rect padding;
	Rect anchors;
	Matrix44 matrix;

	void reset()
	{
		position.reset();
		pivot.reset();
		scale.reset();
		rotation.reset();
	}
};

struct EventAction
{
	enum class CallType
	{
		Callback,
		LuaScript
	};

	enum class Type
	{
		None,
		MouseDown,
		MouseUp,
		MouseClick,
		MouseIn,
		MouseOut,
		KeyDown,
		KeyUp,
		KeyPress,
		GotFocus,
		LostFocus,

		Count
	};

	Type type = Type::None;
	CallType callType = CallType::Callback;
	typedef bool(*EventCallback)(struct Element* element, EventAction* action);
	EventCallback callback = nullptr;
	std::string luaFunctionName;
};

struct AnimationKey
{
	enum class Type
	{
		None,
		PositionX,
		PositionY,
		PositionZ,
		PivotX,
		PivotY,
		PivotZ,
		ScaleX,
		ScaleY,
		ScaleZ,
		RotationX,
		RotationY,
		RotationZ,
		SizeWidth,
		SizeHeight,
		PaddingLeft,
		PaddingTop,
		PaddingRight,
		PaddingBottom,
		AnchorLeft,
		AnchorTop,
		AnchorRight,
		AnchorBottom,
		ColorR,
		ColorG,
		ColorB,
		ColorA,
		Active,
		Visible,

		Count
	};

	f32 value = 0.0f;
	f32 time = 0.0f;
	//TODO: add TCB/easing
};

struct AnimationTrack
{
	AnimationKey::Type keyType = AnimationKey::Type::None;
	std::vector<AnimationKey> keys;

	void animate(f32 time);
};

struct Animation
{
	Element* element = nullptr;
	std::vector<AnimationTrack> tracks;
	bool loop = false;

	void rewind();
	void animate(f32 deltaTime);
};

struct ResizableImage
{
	Image image;
	f32 border = 0;
};

struct Transition
{
	enum class Type
	{
		Color,
		ImageSwap,
		Transform,
		ColorAndTransform,
		Animation,

		Count
	};

	Color color[(int)StateType::Count];
	ResizableImage images[(int)StateType::Count];
	Transform transform[(int)StateType::Count];
	EaseType colorEase[(int)StateType::Count];
	EaseType transformEase[(int)StateType::Count];
	f32 colorMultiplier = 1.0f;
	f32 duration = 0.1f;
	Animation animations[(int)StateType::Count];
};

struct Element
{
	std::string name;
	Element* parent = nullptr;
	std::vector<Element*> children;
	Transform transform, worldTransform;
	bool active = true;
	bool visible = true;
	Color color = Color::white;
	ElementType type = ElementType::None;
	Transition transition;

	Element();
	virtual ~Element();
	bool castRay(const Vec3& start, const Vec3& dir, Vec3& outIntersection, f32& outTimeOnRay);
	Element* findElement(const std::string& name);
	void addChild(Element* element);
	void deleteChild(Element* element);
	void deleteChildren();
	virtual void draw();
	virtual void update();
	virtual bool onMouseDown(const InputEvent3D& ev) { return false; }
	virtual bool onMouseUp(const InputEvent3D& ev) { return false; }
	virtual bool onMouseMove(const InputEvent3D& ev) { return false; }
	virtual bool onKeyDown(const InputEvent3D& ev) { return false; }
	virtual bool onKeyUp(const InputEvent3D& ev) { return false; }
};

struct ButtonElement : Element
{
	ResizableImage bodyImage;

	ButtonElement();
	void update() override;
	void draw() override;
	bool onMouseDown(const InputEvent3D& ev) override;
	bool onMouseUp(const InputEvent3D& ev) override;
	bool onMouseMove(const InputEvent3D& ev) override;
	bool onKeyDown(const InputEvent3D& ev)  override;
	bool onKeyUp(const InputEvent3D& ev)  override;
};

struct ToggleElement : Element
{
	ResizableImage bodyImage, checkMarkImage;

	ToggleElement();
	void update() override;
	void draw() override;
	bool onMouseDown(const InputEvent3D& ev) override;
	bool onMouseUp(const InputEvent3D& ev) override;
	bool onMouseMove(const InputEvent3D& ev) override;
	bool onKeyDown(const InputEvent3D& ev)  override;
	bool onKeyUp(const InputEvent3D& ev)  override;
};

struct TextElement : Element
{
	std::string text;

	TextElement();
	void update() override;
	void draw() override;
	bool onMouseDown(const InputEvent3D& ev) override;
	bool onMouseUp(const InputEvent3D& ev) override;
	bool onMouseMove(const InputEvent3D& ev) override;
	bool onKeyDown(const InputEvent3D& ev)  override;
	bool onKeyUp(const InputEvent3D& ev)  override;
};

struct ImageElement : Element
{
	ResizableImage image;

	ImageElement();
	void update() override;
	void draw() override;
	bool onMouseDown(const InputEvent3D& ev) override;
	bool onMouseUp(const InputEvent3D& ev) override;
	bool onMouseMove(const InputEvent3D& ev) override;
	bool onKeyDown(const InputEvent3D& ev)  override;
	bool onKeyUp(const InputEvent3D& ev)  override;
};

struct SliderElement : Element
{
	// ----[]---------
	ResizableImage bodyImage, knobImage;

	SliderElement();
	void update() override;
	void draw() override;
	bool onMouseDown(const InputEvent3D& ev) override;
	bool onMouseUp(const InputEvent3D& ev) override;
	bool onMouseMove(const InputEvent3D& ev) override;
	bool onKeyDown(const InputEvent3D& ev)  override;
	bool onKeyUp(const InputEvent3D& ev)  override;
};

struct ScrollBarElement : Element
{
	// <---[]-------->
	ResizableImage bodyImage, handleImage, leftArrowImage, rightArrowImage;

	ScrollBarElement();
	void update() override;
	void draw() override;
	bool onMouseDown(const InputEvent3D& ev) override;
	bool onMouseUp(const InputEvent3D& ev) override;
	bool onMouseMove(const InputEvent3D& ev) override;
	bool onKeyDown(const InputEvent3D& ev)  override;
	bool onKeyUp(const InputEvent3D& ev)  override;
};

struct DropDownElement : Element
{
	// [        ][ V ]
	ResizableImage bodyImage, arrowImage, leftArrowImage, rightArrowImage;

	DropDownElement();
	void update() override;
	void draw() override;
	bool onMouseDown(const InputEvent3D& ev) override;
	bool onMouseUp(const InputEvent3D& ev) override;
	bool onMouseMove(const InputEvent3D& ev) override;
	bool onKeyDown(const InputEvent3D& ev)  override;
	bool onKeyUp(const InputEvent3D& ev)  override;
};

struct TextInputElement : Element
{
	// [        ]
	ResizableImage bodyImage;

	TextInputElement();
	void update() override;
	void draw() override;
	bool onMouseDown(const InputEvent3D& ev) override;
	bool onMouseUp(const InputEvent3D& ev) override;
	bool onMouseMove(const InputEvent3D& ev) override;
	bool onKeyDown(const InputEvent3D& ev)  override;
	bool onKeyUp(const InputEvent3D& ev)  override;
};

struct ScrolRectElement : Element
{
	// [          ^]
	// [          #]
	// [          .]
	// [          v]
	// [<...#...>]
	ResizableImage bodyImage, leftArrowImage, rightArrowImage, upArrowImage, downArrowImage, horizontalBarBody, verticalBarBody, horizontalThumbBody, verticalThumbBody;
	bool showHorizontalScrollBar = true;
	bool showVerticalScrollBar = true;
	bool showHorizontalScrollBarAlways = false;
	bool showVerticalScrollBarAlways = false;
	bool showHorizontalArrows = true;
	bool showVerticalArrows = true;

	ScrolRectElement();
	void update() override;
	void draw() override;
	bool onMouseDown(const InputEvent3D& ev) override;
	bool onMouseUp(const InputEvent3D& ev) override;
	bool onMouseMove(const InputEvent3D& ev) override;
	bool onKeyDown(const InputEvent3D& ev)  override;
	bool onKeyUp(const InputEvent3D& ev)  override;
};

struct Canvas
{
	std::string name;
	RenderMode renderMode = RenderMode::ScreenSpace;
	ScaleMode scaleMode = ScaleMode::ConstantPixelSize;
	bool receivesEvents = true;
	f32 scale = 1.0f;
	Point referenceResolution;
	Element root;
};

HORUS_API bool ouiInitialize();
HORUS_API void ouiShutdown();
HORUS_API bool ouiLoadFromFile(const std::string& fileName);
HORUS_API bool ouiSaveToFile(const std::string& fileName);
HORUS_API Canvas* ouiCreateCanvas();
HORUS_API void ouiDeleteCanvas(Canvas* canvas);
HORUS_API void ouiDeleteAll();
HORUS_API Canvas* ouiFindCanvas(const std::string& name);
HORUS_API void ouiUpdate(f32 deltaTime);
HORUS_API void ouiDraw();

}