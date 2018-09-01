#pragma once
#include "types.h"
#include "horus.h"
#include "horus_interfaces.h"
#include <map>

namespace hui
{
class UiFont;
struct UiImage;
class UiAtlas;
struct FontTextSize;

/// How an image is drawn, repeated or stretched across the rectangle
enum class ImageSizingPolicy
{
	Stretch,
	Repeat
};

/// Text styling info
struct TextStyle
{
	FontStyle style = FontStyle::Normal; /// the font face style
	Color backFillColor; /// the text color
	bool underline = false; /// true if underline
	bool backFill = false; /// true if back is filled color
};

/// Vertex buffer data used in rendering the UI
struct VertexBufferData
{
	std::vector<Vertex> vertices;
	u32 drawVertexCount = 0;
	f32 vertexCountGrowFactor = 1.5f;
};

struct DrawCommand
{
	enum class Type
	{
		None,
		DrawRect,
		DrawImageBordered,
		DrawLine,
		DrawPolyLine,
		DrawText,
		DrawInterpolatedColors,
		ClipRect,
		SetAtlas,
		SetColor,
		SetFont,
		SetTextStyle,
		SetLineStyle,
		Callback,

		Count
	};

	struct CmdDrawRect
	{
		Rect rect;
		Rect uvRect;
		bool rotated;
		u32 textureIndex;
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
		Point position;
		char* text;
	};

	struct CmdDrawImageBordered
	{
		Rect rect;
		UiImage* image;
		f32 border;
		f32 scale;
	};

	struct CmdDrawInterpolatedColors
	{
		Rect rect;
		Rect uvRect;
		Color topLeft;
		Color bottomLeft;
		Color topRight;
		Color bottomRight;
	};

	struct CmdCallback
	{
		DrawCommandCallback func;
		void* userdata;
	};

	DrawCommand() {}
	DrawCommand(Type newType) 
		: type(newType)
	{}

	Type type = Type::None;
	i32 zOrder = 0;
	CmdDrawRect drawRect;
	CmdDrawLine drawLine;
	CmdDrawPolyLine drawPolyLine;
	CmdDrawText drawText;
	CmdDrawImageBordered drawImageBordered;
	CmdDrawInterpolatedColors drawInterpolatedColors;
	Rect clipRect;
	bool clipToParent;
	bool popClipRect = false;
	UiAtlas* setAtlas;
	Color setColor;
	UiFont* setFont;
	Color setTextColor;
	TextStyle setTextStyle;
	LineStyle setLineStyle;
	CmdCallback callback;
};

class Renderer
{
public:
	enum class DrawSpectrumBrightness
	{
		On,
		Off
	};

	enum class DrawSpectrumDirection
	{
		Horizontal,
		Vertical
	};

	Renderer();
	virtual ~Renderer();
	void clear(const Color& color);
	Rect pushClipRect(const Rect& rect, bool clipToParent = true);
	void popClipRect();
	const Rect& getClipRect() const { return currentClipRect; }
	void setWindowSize(const Point& size);
	const Point& getWindowSize() const { return windowSize; }
	Rect getWindowRect() const { return { 0, 0, windowSize.x, windowSize.y }; }
	void setZOrder(u32 zorder) { zOrder = zorder; }
	void incrementZOrder() { zOrder++; }
	void decrementZOrder() { zOrder--; }
	u32 getZOrder() const { return zOrder; }
	void beginFrame();
	void endFrame();
	UiFont* getFont() const { return currentFont; }
	u32 getDrawCommandCount() const { return drawCommands.size(); }
	void beginDrawCmdInsertion(u32 index) { drawCmdNextInsertIndex = index; }
	void endDrawCmdInsertion() { drawCmdNextInsertIndex = ~0; }

	// Commands
	void cmdSetColor(const Color& color);
	void cmdSetAtlas(UiAtlas* atlas);
	void cmdSetFont(UiFont* font);
	void cmdSetTextUnderline(bool underline);
	void cmdSetTextBackfill(bool backfill);
	void cmdSetTextBackfillColor(const Color& color);
	void cmdSetLineStyle(const LineStyle& style);
	void cmdDrawImage(UiImage* image, const Point& position);
	void cmdDrawImage(UiImage* image, const Rect& rect);
	void cmdDrawImageBordered(UiImage* image, u32 border, const Rect& rect, f32 scale);
	void cmdDrawImageScaledAligned(UiImage* image, const Rect& rect, HAlignType halign, VAlignType valign, f32 scale);
	void cmdDrawSolidColor(const Rect& rect);
	void cmdDrawInterpolatedColors(const Rect& rect, const Color& topLeft, const Color& bottomLeft, const Color& topRight, const Color& bottomRight);
	void cmdDrawSpectrumColors(const Rect& rect, DrawSpectrumBrightness brightness, DrawSpectrumDirection dir);
	void cmdDrawInterpolatedColorsTopBottom(const Rect& rect, const Color& top, const Color& bottom);
	void cmdDrawInterpolatedColorsLeftRight(const Rect& rect, const Color& left, const Color& right);
	void cmdDrawLine(const Point& a, const Point& b);
	void cmdDrawPolyLine(const Point* points, u32 pointCount, bool closed);
	FontTextSize cmdDrawTextAt(
		Utf8String text,
		const Point& position);
	FontTextSize cmdDrawTextInBox(
		Utf8String text,
		const Rect& rect,
		HAlignType horizontal = HAlignType::Left,
		VAlignType vertical = VAlignType::Top);

public:
	bool skipRender = false;
	bool disableRendering = false;

protected:
	void drawAtlasRegion(bool rotatedUv, const Rect& rect, const Rect& atlasUvRect);
	void drawTextGlyph(UiImage* image, const Point& pos);
	void drawQuad(const Rect& rect, const Rect& uvRect);
	void drawQuadRot90(const Rect& rect, const Rect& uvRect);
	void drawTextInternal(
		Utf8String text,
		const Point& rect);
	void drawInterpolatedColors(
		const Rect& rect,
		const Rect& uvRect,
		const Color& topLeft,
		const Color& bottomLeft,
		const Color& topRight,
		const Color& bottomRight);
	void drawSpectrumColors(const Rect& rect, DrawSpectrumBrightness brightness, DrawSpectrumDirection dir);
	void drawInterpolatedColorsTopBottom(const Rect& rect, const Rect& uvRect, const Color& top, const Color& bottom);
	void drawInterpolatedColorsLeftRight(const Rect& rect, const Rect& uvRect, const Color& left, const Color& right);
	void drawImageBordered(UiImage* image, u32 border, const Rect& rect, f32 scale);
	void drawLine(const Point& a, const Point& b);
	void drawPolyLine(const Point* points, u32 pointCount, bool closed);

	bool clipRectNoRot(Rect& rect, Rect& uvRect);
	bool clipRectRot(Rect& rect, Rect& uvRect);
	bool clipRect(bool rotated, Rect& rect, Rect& uvRect);
	void needToAddVertexCount(u32 count);
	char* addUtf8TextToBuffer(const char* text, u32 sizeBytes);
	void addBatch();
	void addDrawCommand(const DrawCommand& cmd);

	u32 textBufferPosition = 0;
	std::vector<char> textBuffer;
	u32 pointBufferPosition = 0;
	std::vector<Point> pointBuffer;
	std::vector<DrawCommand> drawCommands;
	std::vector<RenderBatch> batches;
	std::vector<Rect> clipRectStack;
	VertexBufferData vertexBufferData;
	VertexBuffer* vertexBuffer = nullptr;
	RenderBatch* currentBatch = nullptr;
	TextStyle currentTextStyle;
	LineStyle currentLineStyle;
	Rect currentClipRect;
	Rect currentViewport;
	UiFont* currentFont = nullptr;
	UiAtlas* currentAtlas = nullptr;
	Point windowSize;
	u32 currentColor;
	i32 zOrder = 0;
	u32 atlasTextureIndex = 0;
	u32 drawCmdNextInsertIndex = ~0;
};

}