#pragma once
#include "horus.h"
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_system.h>
#include <vector>
#include <string>

namespace hui
{
struct SdlInitParams
{
	bool vSync = false;
	SDL_GLContext sdlGlContext = nullptr; // set to a valid SDL GL context
	bool initializeSdl = true; // set to false if you already initialized SDL
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

struct Sdl3InputProvider : InputProvider
{
	Sdl3InputProvider();
	~Sdl3InputProvider();
	void startTextInput(HNativeWindow window, const Rect& imeRect) override;
	void stopTextInput() override;
	bool copyToClipboard(const char* text) override;
	bool pasteFromClipboard(char* outText, u32 maxTextSize) override;
	void processEvents() override;
	void setCurrentWindow(HNativeWindow window) override;
	HNativeWindow getCurrentWindow() override;
	HNativeWindow getFocusedWindow() override;
	HNativeWindow getHoveredWindow() override;
	HNativeWindow createWindow(const char* title, NativeWindowFlags flags, NativeWindowState state, const Rect& rect) override;
	void setWindowTitle(HNativeWindow window, const char* title) override;
	std::string getWindowTitle(HNativeWindow window) override;
	u32 getWindowDisplayIndex(HNativeWindow window) override;
	u32 getDisplayCount() const override;
	DisplayInfo getDisplayInfo(u32 displayIndex) override;
	void setWindowSize(HNativeWindow window, const Point& size) override;
	Point getWindowSize(HNativeWindow window) override;
	void setWindowPosition(HNativeWindow window, const Point& pos) override;
	Point getWindowPosition(HNativeWindow window) override;
	NativeWindowState getWindowState(HNativeWindow window);
	void presentWindow(HNativeWindow window) override;
	void destroyWindow(HNativeWindow window) override;
	void showWindow(HNativeWindow window) override;
	void hideWindow(HNativeWindow window) override;
	void raiseWindow(HNativeWindow window) override;
	void maximizeWindow(HNativeWindow window) override;
	void minimizeWindow(HNativeWindow window) override;
	void setCapture(HNativeWindow window) override;
	void releaseCapture() override;
	Point getAbsoluteMousePosition() override;
	bool isMouseButtonDownNow(MouseButton button) override;
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
	void createSystemCursors();

	bool addedMouseMove = false; /// we only want the first mouse move because otherwise we'll get way too many mouse move events in the queue
	bool ownsGlContext = false; // true if it created the OpenGL context
	bool ownsSdlInit = false; // true if the SDL init happened here
	SDL_Cursor* cursors[SDL_SYSTEM_CURSOR_COUNT] = { nullptr };
	std::vector<SdlWindowProxy*> windows;
	std::vector<SDL_Cursor*> customCursors;
	std::vector<SDL_Surface*> customCursorSurfaces;
	SDL_Window* textInputWindow = nullptr;
	SdlWindowProxy* focusedWindow = nullptr;
	SdlWindowProxy* hoveredWindow = nullptr;
	SdlWindowProxy* currentWindow = nullptr;
	u32 lastTime = SDL_GetTicks();
	f32 deltaTime = 0;
	bool sizeChanged = false;
	SdlInitParams initParams;
};

void initializeSdl(const SdlInitParams& params);

#ifdef _LINUX
extern void makeWindowClickThrough_Linux(SDL_Window* window);
#endif

}