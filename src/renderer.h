#pragma once
#include "types.h"

namespace hui
{
struct Font;
struct Image;
struct Atlas;
struct FontTextSize;

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

/// Vertex buffer data used in rendering the UI
struct VertexBufferData
{
	std::vector<Vertex> vertices;
	u32 drawVertexCount = 0;
	f32 vertexCountGrowFactor = 1.5f;
};

struct DrawCmdLayerSplitter
{
	std::vector<DrawCommandVector> layers;
	size_t currentLayerIndex = 0;

	DrawCmdLayerSplitter();
	~DrawCmdLayerSplitter()	{}
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
	{}

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
		Atlas* setAtlas;
		Rgba32 setColor;
		Font* setFont;
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

struct Renderer
{
	Renderer();
	virtual ~Renderer();
	void setCurrentNativeWindow(HNativeWindow wnd);
	void executeDrawCommands(HNativeWindow wnd);
	Rect pushClipRect(const Rect& rect, bool clipToParent = true);
	void popClipRect();
	const Rect& getClipRect() const { return currentClipRect; }
	void setWindowSize(const Point& size);
	const Point& getWindowSize() const { return windowSize; }
	Rect getWindowRect() const { return { 0, 0, windowSize.x, windowSize.y }; }
	void begin();
	void end();
	Font* getFont() const { return currentFont; }
	
	void pushWindowDrawCmdLayer(DrawCmdLayerType type);
	void popWindowDrawCmdLayer();
	void setDrawCmdLayer(u32 index);
	void pushDrawCmdLayersRequest(u32 count);
	void popDrawCmdLayersRequest();
	void resetWindowContexts();
	inline bool allowRendering() const { return !disableRendering && !skipRender; }

	// Commands
	void cmdCallback(RenderCallback callback);
	void cmdClearBackground(const Rgba32 color);
	void cmdSetColor(const Rgba32 color);
	void cmdSetAtlas(Atlas* atlas);
	void cmdSetFont(Font* font);
	void cmdSetTextUnderline(bool underline);
	void cmdSetTextBackfill(bool backfill);
	void cmdSetTextBackfillColor(const Rgba32 color);
	void cmdSetLineStyle(const LineStyle& style);
	void cmdSetFillStyle(const FillStyle& style);
	void cmdDrawQuad(Image* image, const Point& p1, const Point& p2, const Point& p3, const Point& p4);
	void cmdDrawImage(Image* image, const Point& position, f32 scale);
	void cmdDrawImage(Image* image, const Rect& rect);
	void cmdDrawImage(Image* image, const Rect& rect, const Rect& uvRect);
	void cmdDrawImageBordered(Image* image, u32 border, const Rect& rect, f32 scale);
	void cmdDrawImageScaledAligned(Image* image, const Rect& rect, HAlignType halign, VAlignType valign, f32 scale);
	void cmdDrawImageTiled(Image* image, const Rect& rect, const Point& offset = {}, const Point& scale = {1, 1});
	void cmdDrawRectangle(const Rect& rect);
	void cmdDrawFilledRectangle(const Rect& rect);
	void cmdDrawRectangle4Colors(const Rect& rect, const Rgba32 topLeft, const Rgba32 topRight, const Rgba32 bottomRight, const Rgba32 bottomLeft);
	void cmdDrawLine(const Point& a, const Point& b);
	void cmdDrawPolyLine(const Point* points, u32 pointCount, bool closed);
	void cmdDrawSolidTriangle(const Point& p1, const Point& p2, const Point& p3, const Rgba32 c1, const Rgba32 c2, const Rgba32 c3);
	void cmdDrawTextAt(
		const char* text,
		const Point& position);
	void cmdDrawTextInBox(
		const char* text,
		const Rect& rect,
		HAlignType horizontal = HAlignType::Left,
		VAlignType vertical = VAlignType::Top,
		bool singleLineEllipsis = false);

public:
	bool skipRender = false;
	bool disableRendering = false;
	Point viewportOffset;
	TextStyle currentTextStyle;
	LineStyle currentLineStyle;
	FillStyle currentFillStyle;

	void drawAtlasRegion(bool rotated, const Rect& rect, const Rect& atlasUvRect);
	void drawTextGlyph(Image* image, const Point& pos);
	void drawQuad(Image* image, const Point& p1, const Point& p2, const Point& p3, const Point& p4);
	void drawQuad(const Rect& rect, const Rect& uvRect);
	void drawQuad4Colors(const Rect& rect, const Rect& uvRect, const Rgba32 colTopLeft, const Rgba32 colTopRight, const Rgba32 colBottomRight, const Rgba32 colBottomLeft);
	void drawQuadRot90(const Rect& rect, const Rect& uvRect);
	FontTextSize computeSizeOrDrawText(
		const char* text,
		const Rect& rect,
		HAlignType horizAlign,
		VAlignType vertAlign,
		bool doDraw = false,
		Font* font = nullptr,
		bool singleLineEllipsis = false);

	FontTextSize computeSizeOrDrawText(
		const GlyphCode* const text,
		u32 size,
		const Rect& rect,
		HAlignType horizAlign,
		VAlignType vertAlign,
		bool doDraw = false,
		Font* font = nullptr,
		bool singleLineEllipsis = false);

	void drawImageBordered(Image* image, u32 border, const Rect& rect, f32 scale);
	void drawLine(const Point& a, const Point& b);
	void drawPolyLine(const Point* points, u32 pointCount, bool closed);
	void drawTriangle(const Point& p1, const Point& p2, const Point& p3, const Point& uv1, const Point& uv2, const Point& uv3, const Rgba32 c1, const Rgba32 c2, const Rgba32 c3,
		Image* image);

	bool clipRectNoRot(Rect& rect, Rect& uvRect, Rgba32* colors = nullptr);
	bool clipRectRot(Rect& rect, Rect& uvRect, Rgba32* colors = nullptr);
	bool clipRect(bool rotated, Rect& rect, Rect& uvRect, Rgba32* colors = nullptr);
	void needToAddVertexCount(u32 count);
	char* addUtf8TextToBuffer(const char* text, u32 sizeBytes);
	void addBatch();
	void addDrawCommand(const DrawCommand& cmd);

	struct NativeWindowRenderContext
	{
		u32 textBufferPosition = 0;
		std::vector<char> textBuffer;
		u32 pointBufferPosition = 0;
		std::vector<Point> pointBuffer;
		DrawCommandVector drawCmdLayers[(u32)DrawCmdLayerType::Count];
		std::vector<RenderBatch> batches;
		std::vector<Rect> clipRectStack;
		std::vector<DrawCmdLayerType> drawCmdLayerTypeStack;
		DrawCmdLayerType currentDrawCmdLayer = DrawCmdLayerType::Normal;
	};

	HNativeWindow currentWindow = 0;
	NativeWindowRenderContext* currentWindowContext = nullptr;
	std::unordered_map<HNativeWindow, NativeWindowRenderContext> windowContexts;
	VertexBufferData vertexBufferData;
	VertexBuffer* vertexBuffer = nullptr;
	RenderBatch* currentBatch = nullptr;
	Rect currentClipRect;
	Font* currentFont = nullptr;
	Atlas* currentAtlas = nullptr;
	Point windowSize;
	Rgba32 currentColor = 0xffffffff;
	i32 zOrder = 0;
	u32 atlasTextureIndex = 0;
	struct LineInfo { u32 start; u32 len; f32 width; };
	std::vector<LineInfo> lines;
};

}