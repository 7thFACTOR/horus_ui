#pragma once

namespace hui
{
typedef void(*DrawCommandCallback)(void* userdata);

struct Vertex
{
	Point position;
	Point uv;
	u32 color;
	u32 textureIndex = 0;
};

struct InputProvider
{
	virtual void startTextInput(Window window, const Rect& imeRect) = 0;
	virtual void stopTextInput() = 0;
	virtual bool copyToClipboard(const char* text) = 0;
	virtual bool pasteFromClipboard(char* outText, u32 maxTextSize) = 0;
	virtual void processEvents() = 0;
	virtual void setCurrentWindow(Window window) = 0;
	virtual Window getCurrentWindow() = 0;
	virtual Window getFocusedWindow() = 0;
	virtual Window getHoveredWindow() = 0;
	virtual Window getMainWindow() = 0;
	virtual Window createWindow(
		const char* title, i32 width, i32 height,
		WindowBorder border = WindowBorder::Resizable,
		WindowPositionType positionType = WindowPositionType::Undefined,
		Point customPosition = { 0, 0 },
		bool showInTaskBar = true) = 0;
	virtual void setWindowTitle(Window window, const char* title) = 0;
	virtual void setWindowRect(Window window, const Rect& rect) = 0;
	virtual Rect getWindowRect(Window window) = 0;
	virtual void presentWindow(Window window) = 0;
	virtual void destroyWindow(Window window) = 0;
	virtual void showWindow(Window window) = 0;
	virtual void hideWindow(Window window) = 0;
	virtual void raiseWindow(Window window) = 0;
	virtual void maximizeWindow(Window window) = 0;
	virtual void minimizeWindow(Window window) = 0;
	virtual WindowState getWindowState(Window window) = 0;
	virtual void setCapture(Window window) = 0;
	virtual void releaseCapture() = 0;
	virtual Point getMousePosition() = 0;
	virtual void setCursor(MouseCursorType type) = 0;
	virtual MouseCursor createCustomCursor(Rgba32* pixels, u32 width, u32 height, u32 hotX, u32 hotY) = 0;
	virtual void deleteCustomCursor(MouseCursor cursor) = 0;
	virtual void setCustomCursor(MouseCursor cursor) = 0;
	virtual bool mustQuit() = 0;
	virtual bool wantsToQuit() = 0;
	virtual void cancelQuitApplication() = 0;
	virtual void quitApplication() = 0;
	virtual void shutdown() = 0;
};

struct TextureArray
{
	TextureArray() {}
	TextureArray(u32 count, u32 newWidth, u32 newHeight, Rgba32* pixels) {}
	TextureArray(u32 count, u32 newWidth, u32 newHeight) {}
	virtual ~TextureArray() {}
	virtual void resize(u32 count, u32 newWidth, u32 newHeight) = 0;
	virtual void updateData(Rgba32* pixels) = 0;
	virtual void updateLayerData(u32 textureIndex, Rgba32* pixels) = 0;
	virtual void updateRectData(u32 textureIndex, const Rect& rect, Rgba32* pixels) = 0;
	virtual void destroy() = 0;
	virtual GraphicsApiTexture getHandle() const = 0;
	virtual u32 getWidth() const = 0;
	virtual u32 getHeight() const = 0;
	virtual u32 getCount() const = 0;
};

struct VertexBuffer
{
	VertexBuffer() {}
	VertexBuffer(u32 count, Vertex* vertices) {}
	virtual ~VertexBuffer() {}
	virtual void resize(u32 count) = 0;
	virtual void updateData(Vertex* vertices, u32 startVertexIndex, u32 count) = 0;
	virtual void destroy() = 0;
	virtual GraphicsApiVertexBuffer getHandle() const = 0;
};

struct RenderBatch
{
	enum class PrimitiveType
	{
		TriangleList,
		TriangleStrip,
		TriangleFan
	};

	PrimitiveType primitiveType = PrimitiveType::TriangleList;
	VertexBuffer* vertexBuffer;
	TextureArray* textureArray = nullptr;
	Atlas atlas = nullptr;
	u32 startVertexIndex = 0;
	u32 vertexCount = 0;
	typedef void(*DrawCommandCallback)(void* userdata, RenderBatch& batch);
	DrawCommandCallback function;
};

struct GraphicsProvider
{
    enum class ApiType
    {
        OpenGL,
        Vulkan,
        Direct3D,
        Custom,

        Count
    };

	virtual ~GraphicsProvider() {}
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual ApiType getApiType() const = 0;
	virtual TextureArray* createTextureArray() = 0;
	virtual void deleteTextureArray(TextureArray* texture) = 0;
	virtual VertexBuffer* createVertexBuffer() = 0;
	virtual void deleteVertexBuffer(VertexBuffer* vb) = 0;
	virtual GraphicsApiRenderTarget createRenderTarget(u32 width, u32 height) = 0;
	virtual void destroyRenderTarget(GraphicsApiRenderTarget rt) = 0;
	virtual void setRenderTarget(GraphicsApiRenderTarget rt) = 0;
	virtual void commitRenderState() = 0;
	virtual void setViewport(const Point& windowSize, const Rect& viewport) = 0;
	virtual void clear(const Color& color) = 0;
	virtual void draw(struct RenderBatch* batches, u32 count) = 0;
};

};