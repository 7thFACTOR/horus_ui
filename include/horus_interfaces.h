#pragma once

namespace hui
{
/// A vertex struct for rendering UI
struct Vertex
{
	Point position;
	Point uv;
	u32 color;
	u32 textureIndex = 0; /// what atlas texture index this vertex is using
};

/// The input provider class is used for input and windowing services
struct InputProvider
{
	/// Start text input, usually called by the library to show IME suggestions boxes
	/// \param window the window where the text started to be input
	/// \param imeRect the rectangle where to show the suggestion box
	virtual void startTextInput(Window window, const Rect& imeRect) = 0;

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
	virtual void setCurrentWindow(Window window) = 0;

	/// \return the current native window
	virtual Window getCurrentWindow() = 0;

	/// \return the focused native window
	virtual Window getFocusedWindow() = 0;

	/// \return the hovered native window
	virtual Window getHoveredWindow() = 0;

	/// \return the main native window, this window is the one that upon closing, will end the application
	virtual Window getMainWindow() = 0;

	/// Create a new native window, the first window created will be the main window
	/// \param title the window title, UTF8 text
	/// \param width the window width
	/// \param height the window height
	/// \param border the window border type
	/// \param positionType the window position type
	/// \param customPosition if the positionType is custom, then this is the window's initial position
	/// \param showInTaskBar if true then show a button in the taskbar, for supported OS-es
	/// \return the new window handle
	virtual Window createWindow(
		const char* title, i32 width, i32 height,
		WindowBorder border = WindowBorder::Resizable,
		WindowPositionType positionType = WindowPositionType::Undefined,
		Point customPosition = { 0, 0 },
		bool showInTaskBar = true) = 0;

	/// Set window title
	/// \param window the window
	/// \param title UTF8 text for the title
	virtual void setWindowTitle(Window window, const char* title) = 0;

	/// Set the window rectangle
	/// \param window the window
	/// \param rect the rectangle
	virtual void setWindowRect(Window window, const Rect& rect) = 0;

	/// \param window the window
	/// \return the rectangle of the window
	virtual Rect getWindowRect(Window window) = 0;

	/// Present the backbuffer of the specified window
	/// \param window the window to present
	virtual void presentWindow(Window window) = 0;

	/// Destroy a native window
	/// \param window the window
	virtual void destroyWindow(Window window) = 0;

	/// Show a native window
	/// \param window the window to show
	virtual void showWindow(Window window) = 0;

	/// Hide a native window
	/// \param window the window to hide
	virtual void hideWindow(Window window) = 0;

	/// Bring a native window to front of all windows, on supported OS-es
	/// \param window the window
	virtual void raiseWindow(Window window) = 0;

	/// Maximize a native window
	/// \param window the window
	virtual void maximizeWindow(Window window) = 0;

	/// Minimize a native window
	/// \param window the window
	virtual void minimizeWindow(Window window) = 0;

	/// \return the window state
	virtual WindowState getWindowState(Window window) = 0;

	/// Set the input capture to a specified window
	/// \param window the window
	virtual void setCapture(Window window) = 0;

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
	virtual MouseCursor createCustomCursor(Rgba32* pixels, u32 width, u32 height, u32 hotX, u32 hotY) = 0;

	/// Delete a custom mouse cursor
	/// \param cursor the cursor handle
	virtual void deleteCustomCursor(MouseCursor cursor) = 0;

	/// Set the current mouse cursor to a custom cursor
	/// \param cursor the custom cursor handle
	virtual void setCustomCursor(MouseCursor cursor) = 0;

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

/// A graphics texture array
struct TextureArray
{
	TextureArray() {}
	TextureArray(u32 count, u32 newWidth, u32 newHeight, Rgba32* pixels) {}
	TextureArray(u32 count, u32 newWidth, u32 newHeight) {}
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
	virtual GraphicsApiTexture getHandle() const = 0;

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
	VertexBuffer() {}
	VertexBuffer(u32 count, Vertex* vertices) {}
	virtual ~VertexBuffer() {}

	/// Resize the vertex buffer, it will not keep the old contents
	virtual void resize(u32 count) = 0;

	/// Update the vertex data on a specified range
	/// \param vertices the new vertex data slice
	/// \param startVertexIndex the start vertex index offset
	/// \param count the vertex count to update
	virtual void updateData(Vertex* vertices, u32 startVertexIndex, u32 count) = 0;

	/// \return the graphics API handle for this vertex buffer, you may cast it to the proper handle your graphics API uses
	virtual GraphicsApiVertexBuffer getHandle() const = 0;
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
	VertexBuffer* vertexBuffer; /// which vertex buffer to use for rendering
	TextureArray* textureArray = nullptr; /// which texture array to use for rendering
	Atlas atlas = nullptr; /// handle to the corresponding image atlas
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
		Direct3D,
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
	virtual GraphicsApiRenderTarget createRenderTarget(u32 width, u32 height) = 0;

	/// Delete a render target
	virtual void destroyRenderTarget(GraphicsApiRenderTarget rt) = 0;

	/// Set the current render target
	virtual void setRenderTarget(GraphicsApiRenderTarget rt) = 0;

	/// Set the current viewport and scissor box
	/// \param windowSize the native window's current size
	/// \param viewport the viewport with top-left corner as (0,0)
	virtual void setViewport(const Point& windowSize, const Rect& viewport) = 0;

	/// Clear the current backbuffer with a specified color
	virtual void clear(const Color& color) = 0;

	/// Draw the given render batch array
	virtual void draw(struct RenderBatch* batches, u32 count) = 0;
};

};