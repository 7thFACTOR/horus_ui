#pragma once
#include <horus.h>
#include <vector>
#include <string>
#include <unordered_map>

namespace hui
{
enum class FileSeekMode
{
	Current = 1,
	End = 2,
	Set = 0,
};

struct FileProvider
{
	virtual ~FileProvider(){};
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

/// The input provider class is used for input and windowing services
struct InputProvider
{
	virtual ~InputProvider(){}
	/// Start text input, usually called by the library to show IME suggestions boxes
	/// \param window the window where the text started to be input
	/// \param imeRect the rectangle where to show the suggestion box
	virtual void startTextInput(HOsWindow window, const Rect& imeRect) = 0;

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
	virtual void setCurrentWindow(HOsWindow window) = 0;

	/// \return the current native window
	virtual HOsWindow getCurrentWindow() = 0;

	/// \return the focused native window
	virtual HOsWindow getFocusedWindow() = 0;

	/// \return the hovered native window
	virtual HOsWindow getHoveredWindow() = 0;

	/// \return the main native window, this window is the one that upon closing, will end the application
	virtual HOsWindow getMainWindow() = 0;

	/// Create a new native window, the first window created will be the main window
	/// \param title the window title, UTF8 text
	/// \param width the window width
	/// \param height the window height
	/// \param flags the window flags
	/// \param customPosition if the positionType is custom, then this is the window's initial position
	/// \return the new window handle
	virtual HOsWindow createWindow(const char* title, OsWindowFlags flags, const Rect& rect) = 0;

	/// Set window title
	/// \param window the window
	/// \param title UTF8 text for the title
	virtual void setWindowTitle(HOsWindow window, const char* title) = 0;

	/// Set the window client area size
	/// \param window the window
	/// \param size the width and height
	virtual void setWindowClientSize(HOsWindow window, const Point& size) = 0;

	/// Get the window client area size
	/// \param window the window
	virtual Point getWindowClientSize(HOsWindow window) = 0;

	/// Set the window absolute screen position
	/// \param window the window
	/// \param pos the position
	virtual void setWindowPosition(HOsWindow window, const Point& pos) = 0;

	/// Get the window absolute screen position
	/// \param window the window
	virtual Point getWindowPosition(HOsWindow window) = 0;

	/// Return the window current state  
	virtual OsWindowState getWindowState(HOsWindow window) = 0;
	
	/// Present the backbuffer of the specified window
	/// \param window the window to present
	virtual void presentWindow(HOsWindow window) = 0;

	/// Destroy a native window
	/// \param window the window
	virtual void destroyWindow(HOsWindow window) = 0;

	/// Show a native window
	/// \param window the window to show
	virtual void showWindow(HOsWindow window) = 0;

	/// Hide a native window
	/// \param window the window to hide
	virtual void hideWindow(HOsWindow window) = 0;

	/// Bring a native window to front of all windows, on supported OS-es
	/// \param window the window
	virtual void raiseWindow(HOsWindow window) = 0;

	/// Maximize a native window
	/// \param window the window
	virtual void maximizeWindow(HOsWindow window) = 0;

	/// Minimize a native window
	/// \param window the window
	virtual void minimizeWindow(HOsWindow window) = 0;

	/// \return the window state
	virtual OsWindowState getWindowState(HOsWindow window) = 0;

	/// Set the input capture to a specified window
	/// \param window the window
	virtual void setCapture(HOsWindow window) = 0;

	/// Release capture from the captured window (if any)
	virtual void releaseCapture() = 0;

	/// \return the current mouse position
	virtual Point getMousePosition() = 0;

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

	/// \return true if the user called quitApplication()
	virtual bool mustQuit() = 0;

	/// \return true if the main application window was closed by pressing the close button
	virtual bool wantsToQuit() = 0;

	/// Cancel the quit application, so mustQuit will return false from this point
	virtual void cancelQuitApplication() = 0;

	/// Quit the application, mustQuit will return true from now on
	virtual void quitApplication() = 0;

	/// Shutdown the input provider
	virtual void shutdown() = 0;
};

/// A vertex struct for rendering UI
struct Vertex
{
	Point position;
	Point uv;
	u32 color;
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
	DrawCommandCallback commandCallback;
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
	std::vector<f32> lineHeights; // valid when text size is computed from multiple lines of text
};

struct FontInfo
{
	HFontFace fontFace = 0;
	FontMetrics metrics;
};

struct FontProvider
{
	virtual ~FontProvider(){}
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

};