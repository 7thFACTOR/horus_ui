#pragma once
#include "types.h"

namespace hui
{
struct Font;
struct Image;
struct Atlas;
struct FontTextSize;

/// Vertex buffer data used in rendering the UI
struct VertexBufferData
{
	std::vector<Vertex> vertices;
	u32 drawVertexCount = 0;
	f32 vertexCountGrowFactor = 1.5f;
};

struct Renderer
{
	Renderer();
	virtual ~Renderer();
	void nativeWindowSetCurrent(HNativeWindow wnd);
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
	void resetWindowContexts();
	inline bool allowRendering() const { return !disableRendering && !skipRender; }

	// Commands
	void cmdCallback(RenderCallback callback);
	void cmdClearBackground(const Rgba32 color);
	void cmdSetColor(const Rgba32 color);
	void cmdSetTexture(HTexture textureHandle, u32 width, u32 height);
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
		bool singleLineEllipsis = false,
		bool noWordWrap = false);

public:
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

	struct LineInfo
	{
		u32 start;
		u32 len;
		f32 width;
	};

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
		bool singleLineEllipsis = false,
		bool noWordWrap = false);

	FontTextSize computeSizeOrDrawText(
		const GlyphCode* const text,
		u32 size,
		const Rect& rect,
		HAlignType horizAlign,
		VAlignType vertAlign,
		bool doDraw = false,
		Font* font = nullptr,
		bool singleLineEllipsis = false,
		bool noWordWrap = false);

	void drawImageBordered(Image* image, u32 border, const Rect& rect, f32 scale);
	void drawLine(const Point& a, const Point& b);
	void drawPolyLine(const Point* points, u32 pointCount, bool closed);
	void drawTriangle(const Point& p1, const Point& p2, const Point& p3, const Point& uv1, const Point& uv2, const Point& uv3, const Rgba32 c1, const Rgba32 c2, const Rgba32 c3,
		Image* image);

	bool clipRectNoRot(Rect& rect, Rect& uvRect, Rgba32* colors = nullptr) const;
	bool clipRectRot(Rect& rect, Rect& uvRect, Rgba32* colors = nullptr) const;
	bool clipRect(bool rotated, Rect& rect, Rect& uvRect, Rgba32* colors = nullptr) const;
	void needToAddVertexCount(u32 count);
	char* addUtf8TextToBuffer(const char* text, u32 sizeBytes);
	void addBatch();
	void addDrawCommand(const DrawCommand& cmd);

	bool skipRender = false;
	bool disableRendering = false;
	Point viewportOffset;
	TextStyle currentTextStyle;
	LineStyle currentLineStyle;
	FillStyle currentFillStyle;
	HNativeWindow currentWindow = 0;
	NativeWindowRenderContext* currentWindowContext = nullptr;
	DrawCommandVector* currentDrawCmdLayer = nullptr;
	RenderBatch* currentBatch = nullptr;
	Rect currentClipRect;
	Font* currentFont = nullptr;
	Rgba32 currentColor = 0xffffffff;
	HTexture currentTexture = 0;
	u32 currentTextureWidth = 0, currentTextureHeight = 0;
	Point windowSize;
	VertexBufferData vertexBufferData;
	std::vector<LineInfo> lines;
	std::unordered_map<HNativeWindow, NativeWindowRenderContext> windowContexts;
};

}