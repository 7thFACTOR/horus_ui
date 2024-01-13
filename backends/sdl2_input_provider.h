#pragma once
#include "horus_interfaces.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_version.h>
#include <vector>
#include <string>

namespace hui
{
struct SdlInitParams
{
	bool vSync = true;
	SDL_GLContext sdlGlContext = nullptr; // set to a valid SDL GL context
	bool initializeSdl = true; // set to false if you already initialized SDL
	std::string mainWindowTitle;
	Rect mainWindowRect = {0, 0, 800, 600};
	OsWindowFlags mainWindowFlags = OsWindowFlags::Resizable;
	OsWindowState mainWindowState = OsWindowState::Normal;
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
	void startTextInput(HOsWindow window, const Rect& imeRect) override;
	void stopTextInput() override;
	bool copyToClipboard(const char* text) override;
	bool pasteFromClipboard(char* outText, u32 maxTextSize) override;
	void processEvents() override;
	void setCurrentWindow(HOsWindow window) override;
	HOsWindow getCurrentWindow() override;
	HOsWindow getFocusedWindow() override;
	HOsWindow getHoveredWindow() override;
	HOsWindow getMainWindow() override;
	HOsWindow createWindow(const char* title, OsWindowFlags flags, OsWindowState state, const Rect& rect) override;
	void setWindowTitle(HOsWindow window, const char* title) override;
	std::string getWindowTitle(HOsWindow window) override;
	u32 getWindowDisplayIndex(HOsWindow window) override;
	u32 getDisplayCount() const override;
	DisplayInfo getDisplayInfo(u32 displayIndex) override;
	void setWindowClientSize(HOsWindow window, const Point& size) override;
	Point getWindowClientSize(HOsWindow window) override;
	Rect getWindowRect(HOsWindow window) override;
	void setWindowRect(HOsWindow window, const Rect& rect) override;
	void setWindowPosition(HOsWindow window, const Point& pos) override;
	Point getWindowPosition(HOsWindow window) override;
	OsWindowState getWindowState(HOsWindow window);
	void presentWindow(HOsWindow window) override;
	void destroyWindow(HOsWindow window) override;
	void showWindow(HOsWindow window) override;
	void hideWindow(HOsWindow window) override;
	void raiseWindow(HOsWindow window) override;
	void maximizeWindow(HOsWindow window) override;
	void minimizeWindow(HOsWindow window) override;
	void setCapture(HOsWindow window) override;
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
	void createSystemCursors();

	bool quitApp = false; /// if true, it will end the main app loop
	bool addedMouseMove = false; /// we only want the first mouse move because otherwise we'll get way too many mouse move events in the queue
	bool wantsToQuitApp = false; /// true when the user wants to quit the app
	bool ownsGlContext = false; // true if it created the OpenGL context
	bool ownsSdlInit = false; // true if the SDL init happened here
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
	SdlInitParams initParams;
};

void initializeSdl(const SdlInitParams& params);

}