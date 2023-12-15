#pragma once
#include "horus.h"
#include "horus_interfaces.h"
#include <vector>

#if 0

namespace hui
{
struct SdlInitParams
{
	char* mainWindowTitle = nullptr;
	Rect mainWindowRect;
	WindowFlags windowFlags = WindowFlags::Resizable;
	bool vSync = true;
	void* sdlContext = nullptr;
	SDL_Window* sdlMainWindow = nullptr;
	GraphicsProvider* gfxProvider = nullptr;
	AntiAliasing antiAliasing = AntiAliasing::None;
};

struct SdlWindowProxy
{
	SDL_Window* sdlWindow = nullptr;
	// add here any graphics API aux data (DX11 swapchain etc.)

	~SdlWindowProxy()
	{
		// delete any graphics API objects, when window is destroyed
	}
};

struct Sdl2InputProvider : InputProvider
{
	Sdl2InputProvider();
	~Sdl2InputProvider();
	void startTextInput(Window window, const Rect& imeRect) override;
	void stopTextInput() override;
	bool copyToClipboard(const char* text) override;
	bool pasteFromClipboard(char* outText, u32 maxTextSize) override;
	void processEvents() override;
	void setCurrentWindow(Window window) override;
	Window getCurrentWindow() override;
	Window getFocusedWindow() override;
	Window getHoveredWindow() override;
	Window getMainWindow() override;
	Window createWindow(
		const char* title, i32 width, i32 height,
		WindowFlags flags = WindowFlags::Resizable | WindowFlags::Centered,
		Point customPosition = { 0, 0 }) override;
	void setWindowTitle(Window window, const char* title) override;
	void setWindowRect(Window window, const Rect& rect) override;
	Rect getWindowRect(Window window) override;
	void presentWindow(Window window) override;
	void destroyWindow(Window window) override;
	void showWindow(Window window) override;
	void hideWindow(Window window) override;
	void raiseWindow(Window window) override;
	void maximizeWindow(Window window) override;
	void minimizeWindow(Window window) override;
	WindowState getWindowState(Window window) override;
	void setCapture(Window window) override;
	void releaseCapture() override;
	Point getMousePosition() override;
	bool mustQuit() override;
	bool wantsToQuit() override;
	void cancelQuitApplication() override;
	void quitApplication() override;
	void shutdown() override;
	void setCursor(MouseCursorType type) override;
	MouseCursor createCustomCursor(Rgba32* pixels, u32 width, u32 height, u32 hotX, u32 hotY) override;
	void deleteCustomCursor(MouseCursor cursor) override;
	void setCustomCursor(MouseCursor cursor) override;
	void updateDeltaTime();

	KeyCode fromSdlKey(int code);
	void addSdlEvent(SDL_Event& ev);
	void processSdlEvents();
	SdlWindowProxy* findSdlWindow(SDL_Window* wnd);

	bool quitApp = false; /// if true, it will end the main app loop
	bool addedMouseMove = false; /// we only want the first mouse move because otherwise we'll get way too many mouse move events in the queue
	bool wantsToQuitApp = false; /// true when the user wants to quit the app
	SDL_GLContext sdlOpenGLCtx = nullptr;
	SDL_Cursor *cursors[SDL_NUM_SYSTEM_CURSORS] = { nullptr };
	std::vector<SdlWindowProxy*> windows;
	std::vector<SDL_Cursor*> customCursors;
	std::vector<SDL_Surface*> customCursorSurfaces;
	SdlWindowProxy* mainWindow = nullptr;
	SdlWindowProxy* focusedWindow = nullptr;
	SdlWindowProxy* hoveredWindow = nullptr;
	SdlWindowProxy* currentWindow = nullptr;
	hui::Context context = nullptr;
	u32 lastTime = SDL_GetTicks();
	f32 deltaTime = 0;
	bool sizeChanged = false;
	GraphicsProvider* gfxProvider = nullptr;
};

void initializeWithSDL(const SdlInitParams& settings);

}

#endif
