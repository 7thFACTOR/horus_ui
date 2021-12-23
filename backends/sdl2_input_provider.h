#pragma once
#include "horus_interfaces.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_version.h>
#include <vector>

namespace hui
{
struct SdlSettings
{
	char* mainWindowTitle = nullptr;
	Rect mainWindowRect;
	WindowFlags windowFlags = WindowFlags::Resizable;
	bool vSync = true;
	void* sdlGLContext = nullptr; // set to a valid SDL
	bool initializeSDL = true; // set to false if you already initialized SDL 
	SDL_Window* sdlMainWindow = nullptr;
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
	void startTextInput(HWindow window, const Rect& imeRect) override;
	void stopTextInput() override;
	bool copyToClipboard(const char* text) override;
	bool pasteFromClipboard(char* outText, u32 maxTextSize) override;
	void processEvents() override;
	void setCurrentWindow(HWindow window) override;
	HWindow getCurrentWindow() override;
	HWindow getFocusedWindow() override;
	HWindow getHoveredWindow() override;
	HWindow getMainWindow() override;
	HWindow createWindow(
		const char* title, i32 width, i32 height,
		WindowFlags flags = WindowFlags::Resizable | WindowFlags::Centered,
		Point customPosition = { 0, 0 }) override;
	void setWindowTitle(HWindow window, const char* title) override;
	void setWindowRect(HWindow window, const Rect& rect) override;
	Rect getWindowRect(HWindow window) override;
	void presentWindow(HWindow window) override;
	void destroyWindow(HWindow window) override;
	void showWindow(HWindow window) override;
	void hideWindow(HWindow window) override;
	void raiseWindow(HWindow window) override;
	void maximizeWindow(HWindow window) override;
	void minimizeWindow(HWindow window) override;
	WindowState getWindowState(HWindow window) override;
	void setCapture(HWindow window) override;
	void releaseCapture() override;
	Point getMousePosition() override;
	bool mustQuit() override;
	bool wantsToQuit() override;
	void cancelQuitApplication() override;
	void quitApplication() override;
	void shutdown() override;
	void setCursor(MouseCursorType type) override;
	HMouseCursor createCustomCursor(Rgba32* pixels, u32 width, u32 height, u32 hotX, u32 hotY) override;
	void deleteCustomCursor(HMouseCursor cursor) override;
	void setCustomCursor(HMouseCursor cursor) override;
	void updateDeltaTime();

	KeyCode fromSdlKey(int code);
	void addSdlEvent(SDL_Event& ev);
	void processSdlEvents();
	SdlWindowProxy* findSdlWindow(SDL_Window* wnd);

	bool quitApp = false; /// if true, it will end the main app loop
	bool addedMouseMove = false; /// we only want the first mouse move because otherwise we'll get way too many mouse move events in the queue
	bool wantsToQuitApp = false; /// true when the user wants to quit the app
	bool ownsGLContext = false; // true if it created the OpenGL context
	bool ownsSDLInit = false; // true if the SDL init happened here
	SDL_GLContext sdlGLContext = nullptr;
	SDL_Cursor* cursors[SDL_NUM_SYSTEM_CURSORS] = { nullptr };
	std::vector<SdlWindowProxy*> windows;
	std::vector<SDL_Cursor*> customCursors;
	std::vector<SDL_Surface*> customCursorSurfaces;
	SdlWindowProxy* mainWindow = nullptr;
	SdlWindowProxy* focusedWindow = nullptr;
	SdlWindowProxy* hoveredWindow = nullptr;
	SdlWindowProxy* currentWindow = nullptr;
	u32 lastTime = SDL_GetTicks();
	f32 deltaTime = 0;
	bool sizeChanged = false;
};

void setupSDL(const SdlSettings& settings);

}