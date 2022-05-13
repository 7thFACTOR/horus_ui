#pragma once
#include "types.h"
#include "horus.h"
#include "horus_interfaces.h"
#include <unordered_map>

namespace hui
{
class Font;
struct Image;
class Atlas;
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
		DrawQuad,
		DrawImageBordered,
		DrawLine,
		DrawPolyLine,
		DrawText,
		DrawInterpolatedColors,
		DrawSolidTriangle,
		ClipRect,
		SetViewportOffset,
		SetAtlas,
		SetColor,
		SetFont,
		SetTextStyle,
		SetLineStyle,
		SetFillStyle,
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
		Point position;
		char* text;
	};

	struct CmdDrawImageBordered
	{
		Rect rect;
		Image* image;
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

	struct CmdSetViewportOffset
	{
		Point offset;
	};

	DrawCommand() {}
	DrawCommand(Type newType)
		: type(newType)
	{}

	Type type = Type::None;
	i32 zOrder = 0;
	//TODO: make union of all cmds
	CmdDrawRect drawRect;
	CmdDrawQuad drawQuad;
	CmdDrawLine drawLine;
	CmdDrawPolyLine drawPolyLine;
	CmdDrawText drawText;
	CmdDrawImageBordered drawImageBordered;
	CmdDrawInterpolatedColors drawInterpolatedColors;
	CmdDrawTriangle drawTriangle;
	CmdSetViewportOffset setViewportOffset;
	Rect clipRect;
	bool clipToParent;
	bool popClipRect = false;
	Atlas* setAtlas;
	Color setColor;
	Font* setFont;
	Color setTextColor;
	TextStyle setTextStyle;
	LineStyle setLineStyle;
	FillStyle setFillStyle;
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
	Font* getFont() const { return currentFont; }
	u32 getDrawCommandCount() const { return drawCommands.size(); }
	void beginDrawCmdInsertion(u32 index) { drawCmdNextInsertIndex = index; }
	void endDrawCmdInsertion() { drawCmdNextInsertIndex = ~0; }

	// Commands
	void cmdSetColor(const Color& color);
	void cmdSetAtlas(Atlas* atlas);
	void cmdSetFont(Font* font);
	void cmdSetTextUnderline(bool underline);
	void cmdSetTextBackfill(bool backfill);
	void cmdSetTextBackfillColor(const Color& color);
	void cmdSetLineStyle(const LineStyle& style);
	void cmdSetFillStyle(const FillStyle& style);
	void cmdDrawQuad(Image* image, const Point& p1, const Point& p2, const Point& p3, const Point& p4);
	void cmdDrawImage(Image* image, const Point& position, f32 scale);
	void cmdDrawImage(Image* image, const Rect& rect);
	void cmdDrawImage(Image* image, const Rect& rect, const Rect& uvRect);
	void cmdDrawImageBordered(Image* image, u32 border, const Rect& rect, f32 scale);
	void cmdDrawImageScaledAligned(Image* image, const Rect& rect, HAlignType halign, VAlignType valign, f32 scale);
	void cmdDrawRectangle(const Rect& rect);
	void cmdDrawSolidRectangle(const Rect& rect);
	void cmdDrawInterpolatedColors(const Rect& rect, const Color& topLeft, const Color& bottomLeft, const Color& topRight, const Color& bottomRight);
	void cmdDrawSpectrumColors(const Rect& rect, DrawSpectrumBrightness brightness, DrawSpectrumDirection dir);
	void cmdDrawInterpolatedColorsTopBottom(const Rect& rect, const Color& top, const Color& bottom);
	void cmdDrawInterpolatedColorsLeftRight(const Rect& rect, const Color& left, const Color& right);
	void cmdDrawLine(const Point& a, const Point& b);
	void cmdDrawPolyLine(const Point* points, u32 pointCount, bool closed);
	void cmdDrawSolidTriangle(const Point& p1, const Point& p2, const Point& p3);
	FontTextSize cmdDrawTextAt(
		const char* text,
		const Point& position);
	FontTextSize cmdDrawTextInBox(
		const char* text,
		const Rect& rect,
		HAlignType horizontal = HAlignType::Left,
		VAlignType vertical = VAlignType::Top);

public:
	bool skipRender = false;
	bool disableRendering = false;
	Point viewportOffset;
	TextStyle currentTextStyle;
	LineStyle currentLineStyle;
	FillStyle currentFillStyle;

protected:
	void drawAtlasRegion(bool rotatedUv, const Rect& rect, const Rect& atlasUvRect);
	void drawTextGlyph(Image* image, const Point& pos);
	void drawQuad(Image* image, const Point& p1, const Point& p2, const Point& p3, const Point& p4);
	void drawQuad(const Rect& rect, const Rect& uvRect);
	void drawQuadRot90(const Rect& rect, const Rect& uvRect);
	void drawTextInternal(
		const char* text,
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
	void drawImageBordered(Image* image, u32 border, const Rect& rect, f32 scale);
	void drawLine(const Point& a, const Point& b);
	void drawPolyLine(const Point* points, u32 pointCount, bool closed);
	void drawTriangle(const Point& p1, const Point& p2, const Point& p3, const Point& uv1, const Point& uv2, const Point& uv3, Image* image);

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
	Rect currentClipRect;
	Font* currentFont = nullptr;
	Atlas* currentAtlas = nullptr;
	Point windowSize;
	u32 currentColor;
	i32 zOrder = 0;
	u32 atlasTextureIndex = 0;
	u32 drawCmdNextInsertIndex = ~0;
};

}