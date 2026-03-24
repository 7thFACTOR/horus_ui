#define NOMINMAX
#include "sdl3_input.h"
#include <string.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>
#include <glad/gl.h>
#include "vulkan_graphics.h"
#ifdef _WINDOWS
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#endif
#include <algorithm>

#ifdef _LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#endif

namespace hui
{
struct SdlWindowProxy
{
	SDL_Window* sdlWindow = nullptr;
#ifdef _WINDOWS
	// dx11
	void* dx11SwapChain = nullptr;
	void* dx11RTV = nullptr;
	// dx12
	void* dx12SwapChain = nullptr;
	void* dx12RTVHeap = nullptr;
	void* dx12RTV = nullptr;
	u32 dx12CurrentBackBuffer = 0;
#endif
	// Vulkan surface (if using Vulkan)
	VkSurfaceKHR surface = VK_NULL_HANDLE;
};

#ifdef _WINDOWS
ID3D11Device* g_dx11Device = nullptr;
ID3D11DeviceContext* g_dx11DeviceContext = nullptr;
ID3D12Device* g_dx12Device = nullptr;
ID3D12CommandQueue* g_dx12CommandQueue = nullptr;
ID3D12CommandAllocator* g_dx12CommandAllocator = nullptr;
ID3D12GraphicsCommandList* g_dx12CommandList = nullptr;
#endif

struct Sdl3InputContext
{
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
	Sdl3InitParams initParams;
};

static Sdl3InputContext* sdl3InputContext = nullptr;

#ifdef _WINDOWS
// Make the window click-through on Windows
static void makeWindowClickThrough_Windows(SDL_Window* window) {
	HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	if (!hwnd) {
		printf("Failed to get native window handle: %s\n", SDL_GetError());
		return;
	}
	
	LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
	SetWindowLong(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED | WS_EX_TRANSPARENT);
	SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

#endif

#ifdef _LINUX
namespace hui
{
	void makeWindowClickThrough_Linux(SDL_Window* sdlwindow)
	{
		auto window = (long unsigned int)SDL_GetPointerProperty(SDL_GetWindowProperties(sdlwindow), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, NULL);
		auto display = (Display*)SDL_GetPointerProperty(SDL_GetWindowProperties(sdlwindow), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);

		if (!window)
		{
			printf("Failed to get native X11 window handle: %s\n", SDL_GetError());
			return;
		}

		int opcode, eventBase, errorBase;
		// Set the window to be non-blocking by making it pass through input
		Atom opacityAtom = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);
		unsigned long opacity = 0; // Fully transparent (not for input, just visual)
		XChangeProperty(display, window, opacityAtom, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&opacity, 1);

		// Set the input shape to None (completely transparent to input)
		XShapeCombineMask(display, window, ShapeInput, 0, 0, None, ShapeSet);

		// Use XShape to set the input to be passed through to other windows
		if (XShapeQueryExtension(display, &eventBase, &errorBase)) {
			XShapeCombineMask(display, window, ShapeInput, 0, 0, None, ShapeSet);
		}

		// Map the window (make it visible)
		XMapWindow(display, window);
		// // Grab the pointer to prevent the window from receiving mouse events
		// XGrabPointer(hdisplay, hwnd, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
		//              GrabModeAsync, GrabModeAsync, hwnd, None, CurrentTime);

		// 	XSelectInput(hdisplay, hwnd, 0);
	}

}
#endif

//TODO: make it a map
static KeyCode fromSdlKey(int code)
{
	KeyCode key = KeyCode::None;

	switch (code)
	{
	case SDLK_UNKNOWN: key = KeyCode::None; break;
	case SDLK_RETURN: key = KeyCode::Enter; break;
	case SDLK_ESCAPE: key = KeyCode::Esc; break;
	case SDLK_BACKSPACE: key = KeyCode::Backspace; break;
	case SDLK_TAB: key = KeyCode::Tab; break;
	case SDLK_SPACE: key = KeyCode::Space; break;
	case SDLK_EXCLAIM: key = KeyCode::None; break;
	case SDLK_DBLAPOSTROPHE: key = KeyCode::None; break;
	case SDLK_HASH: key = KeyCode::None; break;
	case SDLK_PERCENT: key = KeyCode::None; break;
	case SDLK_DOLLAR: key = KeyCode::None; break;
	case SDLK_AMPERSAND: key = KeyCode::None; break;
	case SDLK_APOSTROPHE: key = KeyCode::None; break;
	case SDLK_LEFTPAREN: key = KeyCode::LBracket; break;
	case SDLK_RIGHTPAREN: key = KeyCode::RBracket; break;
	case SDLK_ASTERISK: key = KeyCode::None; break;
	case SDLK_PLUS: key = KeyCode::None; break;
	case SDLK_COMMA: key = KeyCode::Comma; break;
	case SDLK_MINUS: key = KeyCode::None; break;
	case SDLK_PERIOD: key = KeyCode::Period; break;
	case SDLK_SLASH: key = KeyCode::Slash; break;
	case SDLK_0: key = KeyCode::Num0; break;
	case SDLK_1: key = KeyCode::Num1; break;
	case SDLK_2: key = KeyCode::Num2; break;
	case SDLK_3: key = KeyCode::Num3; break;
	case SDLK_4: key = KeyCode::Num4; break;
	case SDLK_5: key = KeyCode::Num5; break;
	case SDLK_6: key = KeyCode::Num6; break;
	case SDLK_7: key = KeyCode::Num7; break;
	case SDLK_8: key = KeyCode::Num8; break;
	case SDLK_9: key = KeyCode::Num9; break;
	case SDLK_COLON: key = KeyCode::None; break;
	case SDLK_SEMICOLON: key = KeyCode::Semicolon; break;
	case SDLK_LESS: key = KeyCode::None; break;
	case SDLK_EQUALS: key = KeyCode::Equals; break;
	case SDLK_GREATER: key = KeyCode::None; break;
	case SDLK_QUESTION: key = KeyCode::None; break;
	case SDLK_AT: key = KeyCode::None; break;
	case SDLK_LEFTBRACKET: key = KeyCode::LBracket; break;
	case SDLK_BACKSLASH: key = KeyCode::Backslash; break;
	case SDLK_RIGHTBRACKET: key = KeyCode::RBracket; break;
	case SDLK_CARET: key = KeyCode::None; break;
	case SDLK_UNDERSCORE: key = KeyCode::None; break;
	case SDLK_GRAVE: key = KeyCode::None; break;
	case SDLK_A: key = KeyCode::A; break;
	case SDLK_B: key = KeyCode::B; break;
	case SDLK_C: key = KeyCode::C; break;
	case SDLK_D: key = KeyCode::D; break;
	case SDLK_E: key = KeyCode::E; break;
	case SDLK_F: key = KeyCode::F; break;
	case SDLK_G: key = KeyCode::G; break;
	case SDLK_H: key = KeyCode::H; break;
	case SDLK_I: key = KeyCode::I; break;
	case SDLK_J: key = KeyCode::J; break;
	case SDLK_K: key = KeyCode::K; break;
	case SDLK_L: key = KeyCode::L; break;
	case SDLK_M: key = KeyCode::M; break;
	case SDLK_N: key = KeyCode::N; break;
	case SDLK_O: key = KeyCode::O; break;
	case SDLK_P: key = KeyCode::P; break;
	case SDLK_Q: key = KeyCode::Q; break;
	case SDLK_R: key = KeyCode::R; break;
	case SDLK_S: key = KeyCode::S; break;
	case SDLK_T: key = KeyCode::T; break;
	case SDLK_U: key = KeyCode::U; break;
	case SDLK_V: key = KeyCode::V; break;
	case SDLK_W: key = KeyCode::W; break;
	case SDLK_X: key = KeyCode::X; break;
	case SDLK_Y: key = KeyCode::Y; break;
	case SDLK_Z: key = KeyCode::Z; break;
	case SDLK_CAPSLOCK: key = KeyCode::CapsLock; break;
	case SDLK_F1: key = KeyCode::F1; break;
	case SDLK_F2: key = KeyCode::F2; break;
	case SDLK_F3: key = KeyCode::F3; break;
	case SDLK_F4: key = KeyCode::F4; break;
	case SDLK_F5: key = KeyCode::F5; break;
	case SDLK_F6: key = KeyCode::F6; break;
	case SDLK_F7: key = KeyCode::F7; break;
	case SDLK_F8: key = KeyCode::F8; break;
	case SDLK_F9: key = KeyCode::F9; break;
	case SDLK_F10: key = KeyCode::F10; break;
	case SDLK_F11: key = KeyCode::F11; break;
	case SDLK_F12: key = KeyCode::F12; break;
	case SDLK_PRINTSCREEN: key = KeyCode::PrintScr; break;
	case SDLK_SCROLLLOCK: key = KeyCode::Scroll; break;
	case SDLK_PAUSE: key = KeyCode::Pause; break;
	case SDLK_INSERT: key = KeyCode::Insert; break;
	case SDLK_HOME: key = KeyCode::Home; break;
	case SDLK_PAGEUP: key = KeyCode::PgUp; break;
	case SDLK_DELETE: key = KeyCode::Delete; break;
	case SDLK_END: key = KeyCode::End; break;
	case SDLK_PAGEDOWN: key = KeyCode::PgDown; break;
	case SDLK_RIGHT: key = KeyCode::ArrowRight; break;
	case SDLK_LEFT: key = KeyCode::ArrowLeft; break;
	case SDLK_DOWN: key = KeyCode::ArrowDown; break;
	case SDLK_UP: key = KeyCode::ArrowUp; break;
	case SDLK_NUMLOCKCLEAR: key = KeyCode::NumLock; break;
	case SDLK_KP_DIVIDE: key = KeyCode::Divide; break;
	case SDLK_KP_MULTIPLY: key = KeyCode::Multiply; break;
	case SDLK_KP_MINUS: key = KeyCode::Minus; break;
	case SDLK_KP_PLUS: key = KeyCode::Add; break;
	case SDLK_KP_ENTER: key = KeyCode::Enter; break;
	case SDLK_KP_1: key = KeyCode::NumPad1; break;
	case SDLK_KP_2: key = KeyCode::NumPad2; break;
	case SDLK_KP_3: key = KeyCode::NumPad3; break;
	case SDLK_KP_4: key = KeyCode::NumPad4; break;
	case SDLK_KP_5: key = KeyCode::NumPad5; break;
	case SDLK_KP_6: key = KeyCode::NumPad6; break;
	case SDLK_KP_7: key = KeyCode::NumPad7; break;
	case SDLK_KP_8: key = KeyCode::NumPad8; break;
	case SDLK_KP_9: key = KeyCode::NumPad9; break;
	case SDLK_KP_0: key = KeyCode::NumPad0; break;
	case SDLK_KP_PERIOD: key = KeyCode::None; break;
	case SDLK_APPLICATION: key = KeyCode::Apps; break;
	case SDLK_POWER: key = KeyCode::None; break;
	case SDLK_KP_EQUALS: key = KeyCode::None; break;
	case SDLK_F13: key = KeyCode::None; break;
	case SDLK_F14: key = KeyCode::None; break;
	case SDLK_F15: key = KeyCode::None; break;
	case SDLK_F16: key = KeyCode::None; break;
	case SDLK_F17: key = KeyCode::None; break;
	case SDLK_F18: key = KeyCode::None; break;
	case SDLK_F19: key = KeyCode::None; break;
	case SDLK_F20: key = KeyCode::None; break;
	case SDLK_F21: key = KeyCode::None; break;
	case SDLK_F22: key = KeyCode::None; break;
	case SDLK_F23: key = KeyCode::None; break;
	case SDLK_F24: key = KeyCode::None; break;
	case SDLK_EXECUTE: key = KeyCode::None; break;
	case SDLK_HELP: key = KeyCode::None; break;
	case SDLK_MENU: key = KeyCode::None; break;
	case SDLK_SELECT: key = KeyCode::None; break;
	case SDLK_STOP: key = KeyCode::None; break;
	case SDLK_AGAIN: key = KeyCode::None; break;
	case SDLK_UNDO: key = KeyCode::None; break;
	case SDLK_CUT: key = KeyCode::None; break;
	case SDLK_COPY: key = KeyCode::None; break;
	case SDLK_PASTE: key = KeyCode::None; break;
	case SDLK_FIND: key = KeyCode::None; break;
	case SDLK_MUTE: key = KeyCode::None; break;
	case SDLK_VOLUMEUP: key = KeyCode::None; break;
	case SDLK_VOLUMEDOWN: key = KeyCode::None; break;
	case SDLK_KP_COMMA: key = KeyCode::None; break;
	case SDLK_ALTERASE: key = KeyCode::None; break;
	case SDLK_SYSREQ: key = KeyCode::None; break;
	case SDLK_CANCEL: key = KeyCode::None; break;
	case SDLK_CLEAR: key = KeyCode::None; break;
	case SDLK_PRIOR: key = KeyCode::None; break;
	case SDLK_RETURN2: key = KeyCode::None; break;
	case SDLK_SEPARATOR: key = KeyCode::None; break;
	case SDLK_OUT: key = KeyCode::None; break;
	case SDLK_OPER: key = KeyCode::None; break;
	case SDLK_CLEARAGAIN: key = KeyCode::None; break;
	case SDLK_CRSEL: key = KeyCode::None; break;
	case SDLK_EXSEL: key = KeyCode::None; break;
	case SDLK_KP_00: key = KeyCode::None; break;
	case SDLK_THOUSANDSSEPARATOR: key = KeyCode::None; break;
	case SDLK_DECIMALSEPARATOR: key = KeyCode::None; break;
	case SDLK_CURRENCYUNIT: key = KeyCode::None; break;
	case SDLK_CURRENCYSUBUNIT: key = KeyCode::None; break;
	case SDLK_KP_LEFTPAREN: key = KeyCode::None; break;
	case SDLK_KP_RIGHTPAREN: key = KeyCode::None; break;
	case SDLK_KP_LEFTBRACE: key = KeyCode::None; break;
	case SDLK_KP_RIGHTBRACE: key = KeyCode::None; break;
	case SDLK_KP_TAB: key = KeyCode::None; break;
	case SDLK_KP_BACKSPACE: key = KeyCode::None; break;
	case SDLK_KP_A: key = KeyCode::None; break;
	case SDLK_KP_B: key = KeyCode::None; break;
	case SDLK_KP_C: key = KeyCode::None; break;
	case SDLK_KP_D: key = KeyCode::None; break;
	case SDLK_KP_E: key = KeyCode::None; break;
	case SDLK_KP_F: key = KeyCode::None; break;
	case SDLK_KP_XOR: key = KeyCode::None; break;
	case SDLK_KP_POWER: key = KeyCode::None; break;
	case SDLK_KP_PERCENT: key = KeyCode::None; break;
	case SDLK_KP_LESS: key = KeyCode::None; break;
	case SDLK_KP_GREATER: key = KeyCode::None; break;
	case SDLK_KP_AMPERSAND: key = KeyCode::None; break;
	case SDLK_KP_DBLAMPERSAND: key = KeyCode::None; break;
	case SDLK_KP_VERTICALBAR: key = KeyCode::None; break;
	case SDLK_KP_DBLVERTICALBAR: key = KeyCode::None; break;
	case SDLK_KP_COLON: key = KeyCode::None; break;
	case SDLK_KP_HASH: key = KeyCode::None; break;
	case SDLK_KP_SPACE: key = KeyCode::None; break;
	case SDLK_KP_AT: key = KeyCode::None; break;
	case SDLK_KP_EXCLAM: key = KeyCode::None; break;
	case SDLK_KP_MEMSTORE: key = KeyCode::None; break;
	case SDLK_KP_MEMRECALL: key = KeyCode::None; break;
	case SDLK_KP_MEMCLEAR: key = KeyCode::None; break;
	case SDLK_KP_MEMADD: key = KeyCode::None; break;
	case SDLK_KP_MEMSUBTRACT: key = KeyCode::None; break;
	case SDLK_KP_MEMMULTIPLY: key = KeyCode::None; break;
	case SDLK_KP_MEMDIVIDE: key = KeyCode::None; break;
	case SDLK_KP_PLUSMINUS: key = KeyCode::None; break;
	case SDLK_KP_CLEAR: key = KeyCode::None; break;
	case SDLK_KP_CLEARENTRY: key = KeyCode::None; break;
	case SDLK_KP_BINARY: key = KeyCode::None; break;
	case SDLK_KP_OCTAL: key = KeyCode::None; break;
	case SDLK_KP_DECIMAL: key = KeyCode::None; break;
	case SDLK_KP_HEXADECIMAL: key = KeyCode::None; break;
	case SDLK_LCTRL: key = KeyCode::LControl; break;
	case SDLK_LSHIFT: key = KeyCode::LShift; break;
	case SDLK_LALT: key = KeyCode::LAlt; break;
	case SDLK_LGUI: key = KeyCode::None; break;
	case SDLK_RCTRL: key = KeyCode::RControl; break;
	case SDLK_RSHIFT: key = KeyCode::RShift; break;
	case SDLK_RALT: key = KeyCode::RAlt; break;
	case SDLK_RGUI: key = KeyCode::None; break;
	case SDLK_MODE: key = KeyCode::None; break;
	case SDLK_MEDIA_NEXT_TRACK: key = KeyCode::None; break;
	case SDLK_MEDIA_PREVIOUS_TRACK: key = KeyCode::None; break;
	case SDLK_MEDIA_STOP: key = KeyCode::None; break;
	case SDLK_MEDIA_PLAY: key = KeyCode::None; break;
	case SDLK_MEDIA_PLAY_PAUSE: key = KeyCode::None; break;
	case SDLK_MEDIA_SELECT: key = KeyCode::None; break;
	case SDLK_AC_SEARCH: key = KeyCode::None; break;
	case SDLK_AC_HOME: key = KeyCode::None; break;
	case SDLK_AC_BACK: key = KeyCode::None; break;
	case SDLK_AC_FORWARD: key = KeyCode::None; break;
	case SDLK_AC_STOP: key = KeyCode::None; break;
	case SDLK_AC_REFRESH: key = KeyCode::None; break;
	case SDLK_AC_BOOKMARKS: key = KeyCode::None; break;
	case SDLK_MEDIA_EJECT: key = KeyCode::None; break;
	case SDLK_SLEEP: key = KeyCode::None; break;
	default:
		break;
	}

	return key;
}

static void startTextInput(HNativeWindow window, const Rect& imeRect)
{
	//TODO
	SDL_Rect rc = {0};

	rc.x = imeRect.x;
	rc.y = imeRect.y;
	rc.w = imeRect.width;
	rc.h = imeRect.height;	
	
	if (window)
	{
		sdl3InputContext->textInputWindow = ((SdlWindowProxy*)window)->sdlWindow;
		SDL_SetTextInputArea(sdl3InputContext->textInputWindow, &rc, 0);
		SDL_StartTextInput(sdl3InputContext->textInputWindow);
	}
}

static void stopTextInput()
{
	SDL_StopTextInput(sdl3InputContext->textInputWindow);
}

static bool copyToClipboard(const char* text)
{
	return 0 == SDL_SetClipboardText(text);
}

static bool pasteFromClipboard(char* outText, u32 maxTextSize)
{
	if (!SDL_HasClipboardText())
		return false;

	char* txt = SDL_GetClipboardText();

	if (txt)
	{
		memcpy(outText, txt, std::min((u32)strlen(txt) + 1, maxTextSize));
		SDL_free(txt);
		return true;
	}

	return false;
}

static SdlWindowProxy* findSdlWindow(SDL_Window* wnd)
{
	for (auto& w : sdl3InputContext->windows)
	{
		if (w->sdlWindow == wnd)
			return w;
	}

	return nullptr;
}

static void addSdlEvent(SDL_Event& ev)
{
	InputEvent outEvent = InputEvent();

	switch (ev.type)
	{
	case SDL_EVENT_MOUSE_MOTION:
		if (!sdl3InputContext->addedMouseMove)
		{
			sdl3InputContext->addedMouseMove = true;
			hui::setMouseMoved(true);
			outEvent.type = InputEvent::Type::MouseMove;
			outEvent.mouse.point.x = ev.motion.x;
			outEvent.mouse.point.y = ev.motion.y;
			outEvent.window = findSdlWindow(SDL_GetWindowFromID(ev.motion.windowID));
			auto mods = SDL_GetModState();
			outEvent.mouse.modifiers = KeyModifiers::None;
			outEvent.mouse.modifiers |= (mods & SDL_KMOD_ALT) ? KeyModifiers::Alt : KeyModifiers::None;
			outEvent.mouse.modifiers |= (mods & SDL_KMOD_SHIFT) ? KeyModifiers::Shift : KeyModifiers::None;
			outEvent.mouse.modifiers |= (mods & SDL_KMOD_CTRL) ? KeyModifiers::Control : KeyModifiers::None;
			sdl3InputContext->focusedWindow = (SdlWindowProxy*)outEvent.window;
		}
		else
		{
			return;
		}
		break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	{
		outEvent.type = InputEvent::Type::MouseDown;
		outEvent.mouse.point.x = ev.button.x;
		outEvent.mouse.point.y = ev.button.y;
		outEvent.mouse.button = (MouseButton)(ev.button.button - 1);
		outEvent.mouse.clickCount = ev.button.clicks;
		outEvent.window = findSdlWindow(SDL_GetWindowFromID(ev.button.windowID));
		auto mods = SDL_GetModState();
		outEvent.mouse.modifiers = KeyModifiers::None;
		outEvent.mouse.modifiers |= (mods & SDL_KMOD_ALT) ? KeyModifiers::Alt : KeyModifiers::None;
		outEvent.mouse.modifiers |= (mods & SDL_KMOD_SHIFT) ? KeyModifiers::Shift : KeyModifiers::None;
		outEvent.mouse.modifiers |= (mods & SDL_KMOD_CTRL) ? KeyModifiers::Control : KeyModifiers::None;
		sdl3InputContext->focusedWindow = (SdlWindowProxy*)outEvent.window;
		break;
	}
	case SDL_EVENT_MOUSE_BUTTON_UP:
	{
		outEvent.type = InputEvent::Type::MouseUp;
		outEvent.mouse.point.x = ev.button.x;
		outEvent.mouse.point.y = ev.button.y;
		outEvent.mouse.button = (MouseButton)(ev.button.button - 1);
		outEvent.mouse.clickCount = ev.button.clicks;
		auto mods = SDL_GetModState();
		outEvent.mouse.modifiers = KeyModifiers::None;
		outEvent.mouse.modifiers |= (mods & SDL_KMOD_ALT) ? KeyModifiers::Alt : KeyModifiers::None;
		outEvent.mouse.modifiers |= (mods & SDL_KMOD_SHIFT) ? KeyModifiers::Shift : KeyModifiers::None;
		outEvent.mouse.modifiers |= (mods & SDL_KMOD_CTRL) ? KeyModifiers::Control : KeyModifiers::None;
		outEvent.window = findSdlWindow(SDL_GetWindowFromID(ev.button.windowID));
		break;
	}
	case SDL_EVENT_MOUSE_WHEEL:
	{
		outEvent.type = InputEvent::Type::MouseWheel;
		f32 x, y;
		SDL_GetMouseState(&x, &y);
		outEvent.mouse.point.x = x;
		outEvent.mouse.point.y = y;
		outEvent.mouse.button = (MouseButton)(ev.button.button - 1);
		outEvent.mouse.clickCount = ev.button.clicks;
		outEvent.mouse.wheel.x = ev.wheel.x;
		outEvent.mouse.wheel.y = ev.wheel.y;
		auto mods = SDL_GetModState();
		outEvent.mouse.modifiers = KeyModifiers::None;
		outEvent.mouse.modifiers |= (mods & SDL_KMOD_ALT) ? KeyModifiers::Alt : KeyModifiers::None;
		outEvent.mouse.modifiers |= (mods & SDL_KMOD_SHIFT) ? KeyModifiers::Shift : KeyModifiers::None;
		outEvent.mouse.modifiers |= (mods & SDL_KMOD_CTRL) ? KeyModifiers::Control : KeyModifiers::None;
		outEvent.window = findSdlWindow(SDL_GetWindowFromID(ev.wheel.windowID));
		break;
	}
	case SDL_EVENT_KEY_DOWN:
		outEvent.type = InputEvent::Type::Key;
		outEvent.key.down = true;
		outEvent.key.code = fromSdlKey(ev.key.key);
		outEvent.window = findSdlWindow(SDL_GetWindowFromID(ev.key.windowID));
		outEvent.key.modifiers = KeyModifiers::None;
		outEvent.key.modifiers |= (ev.key.mod & SDL_KMOD_ALT) ? KeyModifiers::Alt : KeyModifiers::None;
		outEvent.key.modifiers |= (ev.key.mod & SDL_KMOD_SHIFT) ? KeyModifiers::Shift : KeyModifiers::None;
		outEvent.key.modifiers |= (ev.key.mod & SDL_KMOD_CTRL) ? KeyModifiers::Control : KeyModifiers::None;
		break;
	case SDL_EVENT_KEY_UP:
		outEvent.type = InputEvent::Type::Key;
		outEvent.key.down = false;
		outEvent.key.code = fromSdlKey(ev.key.key);
		outEvent.window = findSdlWindow(SDL_GetWindowFromID(ev.key.windowID));
		outEvent.key.modifiers = KeyModifiers::None;
		outEvent.key.modifiers |= (ev.key.mod & SDL_KMOD_ALT) ? KeyModifiers::Alt : KeyModifiers::None;
		outEvent.key.modifiers |= (ev.key.mod & SDL_KMOD_SHIFT) ? KeyModifiers::Shift : KeyModifiers::None;
		outEvent.key.modifiers |= (ev.key.mod & SDL_KMOD_CTRL) ? KeyModifiers::Control : KeyModifiers::None;
		break;
	case SDL_EVENT_TEXT_INPUT:
		outEvent.type = InputEvent::Type::Text;
		strcpy(outEvent.text.text, ev.text.text);
		outEvent.window = findSdlWindow(SDL_GetWindowFromID(ev.text.windowID));
		outEvent.key.modifiers = KeyModifiers::None;
		break;
	case SDL_EVENT_DROP_FILE:
		outEvent.type = InputEvent::Type::OsDragDrop;
		outEvent.drop.type = InputEvent::OsDragDropData::Type::DropFile;
		outEvent.drop.filename = new char[strlen(ev.drop.data) + 1];
		strcpy(outEvent.drop.filename, ev.drop.data);
		outEvent.window = findSdlWindow(SDL_GetWindowFromID(ev.drop.windowID));
		break;
	case SDL_EVENT_DROP_TEXT:
		outEvent.type = InputEvent::Type::OsDragDrop;
		outEvent.drop.type = InputEvent::OsDragDropData::Type::DropText;
		outEvent.drop.filename = new char[strlen(ev.drop.data) + 1];
		strcpy(outEvent.drop.filename, ev.drop.data);
		outEvent.window = findSdlWindow(SDL_GetWindowFromID(ev.drop.windowID));
		break;
	case SDL_EVENT_DROP_BEGIN:
		outEvent.type = InputEvent::Type::OsDragDrop;
		outEvent.drop.type = InputEvent::OsDragDropData::Type::DropBegin;
		outEvent.window = findSdlWindow(SDL_GetWindowFromID(ev.drop.windowID));
		break;
	case SDL_EVENT_DROP_COMPLETE:
		outEvent.type = InputEvent::Type::OsDragDrop;
		outEvent.drop.type = InputEvent::OsDragDropData::Type::DropComplete;
		outEvent.window = findSdlWindow(SDL_GetWindowFromID(ev.drop.windowID));
		break;
	case SDL_EVENT_WINDOW_MOVED:
		outEvent.type = InputEvent::Type::WindowMoved;
		break;
	case SDL_EVENT_WINDOW_RESIZED:
	case SDL_EVENT_WINDOW_MAXIMIZED:
	case SDL_EVENT_WINDOW_MINIMIZED:
	case SDL_EVENT_WINDOW_RESTORED:
	case SDL_EVENT_WINDOW_EXPOSED:
	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
	{
		outEvent.type = InputEvent::Type::WindowResized;
		auto proxy = findSdlWindow(SDL_GetWindowFromID(ev.window.windowID));
		if (proxy)
		{
#ifdef _WINDOWS
			if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Direct3D11 && proxy->dx11SwapChain)
			{
				g_dx11DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
				if (proxy->dx11RTV) {
					((ID3D11RenderTargetView*)proxy->dx11RTV)->Release();
					proxy->dx11RTV = nullptr;
				}

				auto sc = (IDXGISwapChain*)proxy->dx11SwapChain;
				sc->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

				ID3D11Texture2D* backBuffer = nullptr;
				sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
				ID3D11RenderTargetView* rtv = nullptr;
				g_dx11Device->CreateRenderTargetView(backBuffer, nullptr, &rtv);
				proxy->dx11RTV = rtv;
				backBuffer->Release();
			}
			else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Direct3D12 && proxy->dx12SwapChain)
			{
				auto sc = (IDXGISwapChain3*)proxy->dx12SwapChain;
				
				ID3D12Resource* b1 = nullptr;
				ID3D12Resource* b2 = nullptr;
				sc->GetBuffer(0, IID_PPV_ARGS(&b1));
				sc->GetBuffer(1, IID_PPV_ARGS(&b2));
				
				extern void dx12PreResize(ID3D12Resource** buffers, int count);
				ID3D12Resource* buffers[2] = { b1, b2 };
				dx12PreResize(buffers, 2);
				
				if (b1) b1->Release();
				if (b2) b2->Release();

				sc->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

				auto rtvHeap = (ID3D12DescriptorHeap*)proxy->dx12RTVHeap;
				SIZE_T rtvDescriptorSize = g_dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
				for (UINT n = 0; n < 2; n++) {
					ID3D12Resource* backBuffer = nullptr;
					sc->GetBuffer(n, IID_PPV_ARGS(&backBuffer));
					g_dx12Device->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);
					backBuffer->Release();
					rtvHandle.ptr += rtvDescriptorSize;
				}
				proxy->dx12CurrentBackBuffer = sc->GetCurrentBackBufferIndex();
			}
#endif
		}
		break;
	}
	case SDL_EVENT_WINDOW_FOCUS_GAINED:
	{
		outEvent.type = InputEvent::Type::WindowGotFocus;
		sdl3InputContext->focusedWindow = findSdlWindow(SDL_GetWindowFromID(ev.window.windowID));
		break;
	}
	case SDL_EVENT_WINDOW_MOUSE_ENTER:
	{
		outEvent.type = InputEvent::Type::WindowMouseEnter;
		sdl3InputContext->hoveredWindow = findSdlWindow(SDL_GetWindowFromID(ev.window.windowID));
		break;
	}
	case SDL_EVENT_WINDOW_MOUSE_LEAVE:
	{
		outEvent.type = InputEvent::Type::WindowMouseLeave;
		sdl3InputContext->hoveredWindow = nullptr;
		break;
	}
	case SDL_EVENT_WINDOW_FOCUS_LOST:
		outEvent.type = InputEvent::Type::WindowLostFocus;
		sdl3InputContext->focusedWindow = nullptr;
		break;
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
	{
		outEvent.type = InputEvent::Type::WindowClose;
		break;
	}
	default:
		break;
	}

	outEvent.window = findSdlWindow(SDL_GetWindowFromID(ev.window.windowID));

	if (outEvent.type != InputEvent::Type::None)
		addInputEvent(outEvent);
}

static void processSdlEvents()
{
	SDL_Event ev;

	sdl3InputContext->addedMouseMove = false;

	while (SDL_PollEvent(&ev))
	{
		addSdlEvent(ev);
	}
}

static void updateDeltaTime()
{
	u32 ticks = SDL_GetTicks();
	sdl3InputContext->deltaTime = (f32)(ticks - sdl3InputContext->lastTime) / 1000.0f;
	sdl3InputContext->lastTime = ticks;
}

static void processWindowEvents()
{
	updateDeltaTime();
	processSdlEvents();
}

f32 getSdl3DeltaTime()
{
	return sdl3InputContext->deltaTime;
}

static void setCursor(MouseCursorType type)
{
	SDL_SetCursor(sdl3InputContext->cursors[(int)type]);
}

static HMouseCursor createCustomCursor(Rgba32* pixels, u32 width, u32 height, u32 hotX, u32 hotY)
{
	SDL_Surface* surf = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * 4);

	auto cur = SDL_CreateColorCursor(surf, hotX, hotY);

	sdl3InputContext->customCursors.push_back(cur);
	sdl3InputContext->customCursorSurfaces.push_back(surf);

	return cur;
}

static void deleteCustomCursor(HMouseCursor cursor)
{
	for (size_t i = 0; i < sdl3InputContext->customCursors.size(); i++)
	{
		if (sdl3InputContext->customCursors[i] == cursor)
		{
			SDL_DestroyCursor(sdl3InputContext->customCursors[i]);
			SDL_DestroySurface(sdl3InputContext->customCursorSurfaces[i]);
			sdl3InputContext->customCursors.erase(sdl3InputContext->customCursors.begin() + i);
			sdl3InputContext->customCursorSurfaces.erase(sdl3InputContext->customCursorSurfaces.begin() + i);
			break;
		}
	}
}

static void setCustomCursor(HMouseCursor cursor)
{
	SDL_SetCursor((SDL_Cursor*)cursor);
}

static void setCurrentWindow(HNativeWindow window)
{
	if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::OpenGL)
	{
		SDL_GL_MakeCurrent(((SdlWindowProxy*)window)->sdlWindow, sdl3InputContext->initParams.sdlGlContext);
		SDL_GL_SetSwapInterval(sdl3InputContext->initParams.vSync ? 1 : 0);
	}
	else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Direct3D11)
	{
#ifdef _WINDOWS
		auto proxy = (SdlWindowProxy*)window;
		if (proxy->dx11RTV) {
			ID3D11RenderTargetView* rtv[] = { (ID3D11RenderTargetView*)proxy->dx11RTV };
			g_dx11DeviceContext->OMSetRenderTargets(1, rtv, nullptr);
		}
#endif
	}
	else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Direct3D12)
	{
#ifdef _WINDOWS
		auto proxy = (SdlWindowProxy*)window;
		if (proxy->dx12RTVHeap && proxy->dx12SwapChain) {
			auto sc = (IDXGISwapChain3*)proxy->dx12SwapChain;
			proxy->dx12CurrentBackBuffer = sc->GetCurrentBackBufferIndex();
			
			auto heap = (ID3D12DescriptorHeap*)proxy->dx12RTVHeap;
			SIZE_T size = g_dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();
			handle.ptr += size * proxy->dx12CurrentBackBuffer;
			
			ID3D12Resource* backBuffer = nullptr;
			sc->GetBuffer(proxy->dx12CurrentBackBuffer, IID_PPV_ARGS(&backBuffer));
			
			extern void dx12SetCurrentRenderTarget(SIZE_T rtvPtr, ID3D12Resource* backBuffer);
			dx12SetCurrentRenderTarget(handle.ptr, backBuffer);
			if (backBuffer) backBuffer->Release();
		}
#endif
	}
	else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Vulkan)
	{
		auto proxy = (SdlWindowProxy*)window;
		hui::vulkanSetCurrentWindow(proxy ? proxy->sdlWindow : nullptr);
	}

	sdl3InputContext->currentWindow = ((SdlWindowProxy*)window);
}

static HNativeWindow getCurrentWindow()
{
	return sdl3InputContext->currentWindow;
}

static HNativeWindow getFocusedWindow()
{
	return sdl3InputContext->focusedWindow;
}

static HNativeWindow getHoveredWindow()
{
	return sdl3InputContext->hoveredWindow;
}

// Hit-test callback that makes the window transparent to mouse events
static SDL_HitTestResult hitTestCallback(SDL_Window* win, const SDL_Point* area, void* data) {
	return SDL_HITTEST_NORMAL; // Ignore input, pass through to windows underneath
}

static HNativeWindow createWindow(
	const char* title, NativeWindowFlags flags, NativeWindowState state, const Rect& rect)
{
	int sdlflags = 0;

	if (state == NativeWindowState::Maximized)
		sdlflags |= SDL_WINDOW_MAXIMIZED;

	if (state == NativeWindowState::Minimized)
		sdlflags |= SDL_WINDOW_MINIMIZED;

	if (state == NativeWindowState::Hidden)
		sdlflags |= SDL_WINDOW_HIDDEN;

	if (has(flags, NativeWindowFlags::Resizable))
		sdlflags |= SDL_WINDOW_RESIZABLE;

	if (has(flags, NativeWindowFlags::NoDecoration))
		sdlflags |= SDL_WINDOW_BORDERLESS;

	if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::OpenGL)
		sdlflags |= SDL_WINDOW_OPENGL;

	if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Vulkan)
		sdlflags |= SDL_WINDOW_VULKAN;

	if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Metal)
		sdlflags |= SDL_WINDOW_METAL;

	auto wnd = SDL_CreateWindow(
		title, rect.width, rect.height,
		sdlflags);

	auto newWnd = new SdlWindowProxy();

	newWnd->sdlWindow = wnd;
	sdl3InputContext->windows.push_back(newWnd);

	sdl3InputContext->focusedWindow = newWnd;
	sdl3InputContext->currentWindow = newWnd;

	if (has(flags, NativeWindowFlags::NoInput))
	{
#ifdef _WINDOWS
		makeWindowClickThrough_Windows(wnd);
#endif
		
#ifdef _LINUX
		makeWindowClickThrough_Linux(wnd);
#endif
	}

	// if no GL context provided, create one for this first window
	if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::OpenGL && !sdl3InputContext->initParams.sdlGlContext)
	{
		sdl3InputContext->initParams.sdlGlContext = SDL_GL_CreateContext(wnd);
		sdl3InputContext->ownsGlContext = true;

		if (!sdl3InputContext->initParams.sdlGlContext)
		{
			printf("Cannot create GL context for SDL: %s\n", SDL_GetError());
		}

		SDL_GL_MakeCurrent(wnd, sdl3InputContext->initParams.sdlGlContext);
		SDL_GL_SetSwapInterval(sdl3InputContext->initParams.vSync ? 1 : 0);

		if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
		{
			printf("GLAD cannot init GL func ptrs\n");
		}

		const GLubyte* renderer = glGetString(GL_RENDERER);  // Get renderer string
		const GLubyte* version = glGetString(GL_VERSION);    // Get version string
		printf("GL Renderer: %s\n", renderer);
		printf("GL Version: %s\n", version);
	}
	else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Direct3D11)
	{
#ifdef _WINDOWS
		HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(wnd), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
		if (!g_dx11Device) {
			UINT createDeviceFlags = 0;
#ifdef _DEBUG
			createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
			D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
			D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, &featureLevel, 1, D3D11_SDK_VERSION, &g_dx11Device, nullptr, &g_dx11DeviceContext);
		}

		DXGI_SWAP_CHAIN_DESC sd{};
		sd.BufferCount = 2;
		sd.BufferDesc.Width = rect.width;
		sd.BufferDesc.Height = rect.height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hwnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		IDXGIDevice* dxgiDevice = nullptr;
		g_dx11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
		IDXGIAdapter* dxgiAdapter = nullptr;
		dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);
		IDXGIFactory* dxgiFactory = nullptr;
		dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

		IDXGISwapChain* swapchain = nullptr;
		dxgiFactory->CreateSwapChain(g_dx11Device, &sd, &swapchain);
		newWnd->dx11SwapChain = swapchain;

		ID3D11Texture2D* backBuffer = nullptr;
		swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
		ID3D11RenderTargetView* rtv = nullptr;
		g_dx11Device->CreateRenderTargetView(backBuffer, nullptr, &rtv);
		newWnd->dx11RTV = rtv;
		backBuffer->Release();

		dxgiFactory->Release();
		dxgiAdapter->Release();
		dxgiDevice->Release();
#endif
	}
	else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Direct3D12)
	{
#ifdef _WINDOWS
		HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(wnd), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
		if (!g_dx12Device) {
#if defined(_DEBUG)
			ID3D12Debug* debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
				debugController->EnableDebugLayer();
				debugController->Release();
			}
#endif
			IDXGIFactory4* factory = nullptr;
			CreateDXGIFactory1(IID_PPV_ARGS(&factory));
			IDXGIAdapter1* adapter = nullptr;
			for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);
				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
				if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_dx12Device)))) break;
				adapter->Release(); adapter = nullptr;
			}
			if (g_dx12Device) {
				D3D12_COMMAND_QUEUE_DESC queueDesc{};
				queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
				queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
				g_dx12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_dx12CommandQueue));
				g_dx12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_dx12CommandAllocator));
				g_dx12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_dx12CommandAllocator, nullptr, IID_PPV_ARGS(&g_dx12CommandList));
				g_dx12CommandList->Close();
			}
			if (adapter) adapter->Release();
			if (factory) factory->Release();
		}

		if (g_dx12CommandQueue) {
			DXGI_SWAP_CHAIN_DESC1 sd{};
		 sd.BufferCount = 2;
			sd.Width = rect.width;
			sd.Height = rect.height;
			sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			sd.SampleDesc.Count = 1;

			IDXGIFactory4* factory = nullptr;
			CreateDXGIFactory1(IID_PPV_ARGS(&factory));
			IDXGISwapChain1* swapChain1 = nullptr;
			factory->CreateSwapChainForHwnd(g_dx12CommandQueue, hwnd, &sd, nullptr, nullptr, &swapChain1);
			IDXGISwapChain3* swapchain = nullptr;
			swapChain1->QueryInterface(IID_PPV_ARGS(&swapchain));
			newWnd->dx12SwapChain = swapchain;
			swapChain1->Release();
			factory->Release();

			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
			rtvHeapDesc.NumDescriptors = 2;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ID3D12DescriptorHeap* rtvHeap = nullptr;
			g_dx12Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
			newWnd->dx12RTVHeap = rtvHeap;

			SIZE_T rtvDescriptorSize = g_dx12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
			for (UINT n = 0; n < 2; n++) {
				ID3D12Resource* backBuffer = nullptr;
				swapchain->GetBuffer(n, IID_PPV_ARGS(&backBuffer));
				g_dx12Device->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);
				backBuffer->Release();
				rtvHandle.ptr += rtvDescriptorSize;
			}
			newWnd->dx12CurrentBackBuffer = swapchain->GetCurrentBackBufferIndex();
		}
#endif
	}

	SDL_SetWindowPosition(wnd, rect.x, rect.y);
	SDL_SyncWindow(wnd);
	SDL_RaiseWindow(wnd);

	// If using Vulkan, and the Vulkan backend is initialized, create a VkSurface for this SDL window
	if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Vulkan)
	{
		// create the surface via the Vulkan backend helper (if it's initialized)
		if (hui::isVulkanInitialized())
		{
			VkSurfaceKHR surf = VK_NULL_HANDLE;
			if (hui::createSurfaceForSdlWindow(wnd, &surf))
			{
				newWnd->surface = surf;

				// Create swapchain for this window (use rect for initial size and sdl init vSync setting)
				if (!hui::createSwapchainForWindow(wnd, surf, rect.width, rect.height, sdl3InputContext->initParams.vSync))
				{
					printf("Warning: failed to create Vulkan swapchain for SDL window\n");
				}
			}
			else
			{
				printf("Warning: failed to create Vulkan surface for SDL window\n");
			}
		}
		else
		{
			// Vulkan not initialized yet - the Vulkan backend should create swapchain later when initVulkan runs.
			// We keep sdlWindow pointer and allow the backend to create swapchain after init.
		}
	}

	return newWnd;
}

static void setWindowTitle(HNativeWindow window, const char* title)
{
	SDL_SetWindowTitle(((SdlWindowProxy*)window)->sdlWindow, title);
}

static u32 getWindowDisplayIndex(HNativeWindow window)
{
	return SDL_GetDisplayForWindow(((SdlWindowProxy*)window)->sdlWindow);
}

static u32 getDisplayCount()
{
	i32 count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&count);
	
	SDL_free(displays);

	return count;
}

static DisplayInfo getDisplayInfo(u32 displayIndex)
{
	DisplayInfo info;

	info.name = SDL_GetDisplayName(displayIndex);
	info.index = displayIndex;
	SDL_Rect rc;
	SDL_GetDisplayBounds(displayIndex, &rc);
	info.bounds = { (f32)rc.x, (f32)rc.y, (f32)rc.w, (f32)rc.h };
	SDL_GetDisplayUsableBounds(displayIndex, &rc);
	info.usableBounds = { (f32)rc.x, (f32)rc.y, (f32)rc.w, (f32)rc.h };

	info.scale = SDL_GetDisplayContentScale(displayIndex);
	
	return info;
}

static void setWindowSize(HNativeWindow window, const Point& size)
{
	SDL_SetWindowSize(((SdlWindowProxy*)window)->sdlWindow, size.x, size.y);
}

static Point getWindowSize(HNativeWindow window)
{
	int w = 0, h = 0;

	SDL_SyncWindow(((SdlWindowProxy*)window)->sdlWindow);
	SDL_GetWindowSize(((SdlWindowProxy*)window)->sdlWindow, &w, &h);

	return { (f32)w, (f32)h };
}

static void setWindowPosition(HNativeWindow window, const Point& pos)
{
	SDL_SetWindowPosition(((SdlWindowProxy*)window)->sdlWindow, pos.x, pos.y);
}

static Point getWindowPosition(HNativeWindow window)
{
	int x = 0, y = 0;

	SDL_GetWindowPosition(((SdlWindowProxy*)window)->sdlWindow, &x, &y);

	return { (f32)x, (f32)y };
}

static void presentWindow(HNativeWindow window)
{
	auto proxy = (SdlWindowProxy*)window;
	if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::OpenGL)
	{
		SDL_GL_SwapWindow(proxy->sdlWindow);
	}
	else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Direct3D11)
	{
#ifdef _WINDOWS
		if (proxy->dx11SwapChain)
			((IDXGISwapChain*)proxy->dx11SwapChain)->Present(sdl3InputContext->initParams.vSync ? 1 : 0, 0);
#endif
	}
	else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Direct3D12)
	{
#ifdef _WINDOWS
		if (proxy->dx12SwapChain) {
			auto sc = (IDXGISwapChain3*)proxy->dx12SwapChain;
			ID3D12Resource* backBuffer = nullptr;
			sc->GetBuffer(proxy->dx12CurrentBackBuffer, IID_PPV_ARGS(&backBuffer));
			
			extern void dx12PreparePresent(ID3D12Resource* backBuffer);
			dx12PreparePresent(backBuffer);
			if (backBuffer) backBuffer->Release();
			
			sc->Present(sdl3InputContext->initParams.vSync ? 1 : 0, 0);
			proxy->dx12CurrentBackBuffer = sc->GetCurrentBackBufferIndex();
		}
#endif
	}
	else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Vulkan)
	{
		// Present via Vulkan backend
		if (proxy->surface != VK_NULL_HANDLE)
		{
			if (!hui::presentSwapchainForWindow(proxy->sdlWindow))
			{
				// If presentation failed, fallback to no-op (swapchain may need recreation)
				// The backend will log errors / handle recreation if implemented.
			}
		}
	}
}

static void destroyWindow(HNativeWindow window)
{
	// if Vulkan surface was created, destroy swapchain then surface
	if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Vulkan)
	{
		auto proxy = (SdlWindowProxy*)window;
		if (proxy->surface != VK_NULL_HANDLE)
		{
			// Destroy any backend swapchain/resources associated with this SDL window first
			hui::destroySwapchainForWindow(((SdlWindowProxy*)window)->sdlWindow);

			hui::destroySurface(proxy->surface);
			proxy->surface = VK_NULL_HANDLE;
		}
	}

	SDL_DestroyWindow(((SdlWindowProxy*)window)->sdlWindow);
	auto iter = std::find(sdl3InputContext->windows.begin(), sdl3InputContext->windows.end(), window);

	if (iter != sdl3InputContext->windows.end())
	{
		delete ((SdlWindowProxy*)window);
		sdl3InputContext->windows.erase(iter);
	}
}

static void showWindow(HNativeWindow window)
{
	SDL_ShowWindow(((SdlWindowProxy*)window)->sdlWindow);
}

static void hideWindow(HNativeWindow window)
{
	SDL_HideWindow(((SdlWindowProxy*)window)->sdlWindow);
}

static void raiseWindow(HNativeWindow window)
{
	SDL_RaiseWindow(((SdlWindowProxy*)window)->sdlWindow);
}

static void maximizeWindow(HNativeWindow window)
{
	SDL_MaximizeWindow(((SdlWindowProxy*)window)->sdlWindow);
}

static void minimizeWindow(HNativeWindow window)
{
	SDL_MinimizeWindow(((SdlWindowProxy*)window)->sdlWindow);
}

static NativeWindowState getWindowState(HNativeWindow window)
{
	auto flags = SDL_GetWindowFlags(((SdlWindowProxy*)window)->sdlWindow);

	if (flags & SDL_WINDOW_HIDDEN)
	{
		return NativeWindowState::Hidden;
	}

	if (flags & SDL_WINDOW_MINIMIZED)
	{
		return NativeWindowState::Minimized;
	}

	if (flags & SDL_WINDOW_MAXIMIZED)
	{
		return NativeWindowState::Maximized;
	}

	return NativeWindowState::Normal;
}

static void setCapture(HNativeWindow window)
{
	SDL_CaptureMouse(true);
}

static void releaseCapture()
{
	SDL_CaptureMouse(false);
}

static Point getAbsoluteMousePosition()
{
	f32 x, y;

	SDL_GetGlobalMouseState(&x, &y);

	return { (f32)x , (f32)y };
}

static bool isMouseButtonDownNow(MouseButton button)
{
	f32 x = 0, y = 0;
	auto buttons = SDL_GetGlobalMouseState(&x, &y);

	if (button == MouseButton::Left) return buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT);
	if (button == MouseButton::Middle) return buttons & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE);
	if (button == MouseButton::Right) return buttons & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT);

	return false;
}

static void createSystemCursors()
{
	for (int i = 0; i < SDL_SYSTEM_CURSOR_COUNT; i++)
	{
		sdl3InputContext->cursors[i] = SDL_CreateSystemCursor((SDL_SystemCursor)i);
	}
}

void initSdl3(Services& services, const Sdl3InitParams& params)
{
	sdl3InputContext = new Sdl3InputContext();
	sdl3InputContext->initParams = params;

	printf("Initializing SDL3 service...\n");

	if (params.initializeSdl)
	{
		SDL_SetMainReady();
		
		auto ok = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS |
                SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD | SDL_INIT_SENSOR);

		if (!ok)
		{
			printf("SDL initialize error: %s\n", SDL_GetError());
			return;
		}

		sdl3InputContext->ownsSdlInit = true;
	}

	if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::OpenGL)
	{
		//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	}
	else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Direct3D11)
	{
		// TODO: Init any DX11 specific SDL hints
	}
	else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Direct3D12)
	{
		// TODO: Init any DX12 specific SDL hints
	}

	createSystemCursors();
	SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "0");

	services.startTextInput = startTextInput;
	services.stopTextInput = stopTextInput;
	services.copyToClipboard = copyToClipboard;
	services.pasteFromClipboard = pasteFromClipboard;
	services.processWindowEvents = processWindowEvents;
	services.setCurrentWindow = setCurrentWindow;
	services.getCurrentWindow = getCurrentWindow;
	services.getFocusedWindow = getFocusedWindow;
	services.getHoveredWindow = getHoveredWindow;
	services.createWindow = createWindow;
	services.setWindowTitle = setWindowTitle;
	services.getWindowDisplayIndex = getWindowDisplayIndex;
	services.getDisplayCount = getDisplayCount;
	services.getDisplayInfo = getDisplayInfo;
	services.setWindowSize = setWindowSize;
	services.getWindowSize = getWindowSize;
	services.setWindowPosition = setWindowPosition;
	services.getWindowPosition = getWindowPosition;
	services.getWindowState = getWindowState;
	services.presentWindow = presentWindow;
	services.destroyWindow = destroyWindow;
	services.showWindow = showWindow;
	services.hideWindow = hideWindow;
	services.raiseWindow = raiseWindow;
	services.maximizeWindow = maximizeWindow;
	services.minimizeWindow = minimizeWindow;
	services.setCapture = setCapture;
	services.releaseCapture = releaseCapture;
	services.getAbsoluteMousePosition = getAbsoluteMousePosition;
	services.isMouseButtonDownNow = isMouseButtonDownNow;
	services.setCursor = setCursor;
	services.createCustomCursor = createCustomCursor;
	services.deleteCustomCursor = deleteCustomCursor;
	services.setCustomCursor = setCustomCursor;
}

void shutdownSdl3(Services& services)
{
	if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::OpenGL)
	{
		if (sdl3InputContext->ownsGlContext)
			SDL_GL_DestroyContext(sdl3InputContext->initParams.sdlGlContext);
	}
	else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Direct3D11)
	{
		// TODO: Clean up DX11 specific context
	}
	else if (sdl3InputContext->initParams.gfxApi == Sdl3GfxApi::Direct3D12)
	{
		// TODO: Clean up DX12 specific context
	}

	if (sdl3InputContext->ownsSdlInit)
		SDL_Quit();

	for (size_t i = 0; i < sdl3InputContext->customCursors.size(); i++)
	{
		SDL_DestroyCursor(sdl3InputContext->customCursors[i]);
		SDL_DestroySurface(sdl3InputContext->customCursorSurfaces[i]);
	}

	delete sdl3InputContext;
	sdl3InputContext = nullptr;

	services.startTextInput = nullptr;
	services.stopTextInput = nullptr;
	services.copyToClipboard = nullptr;
	services.pasteFromClipboard = nullptr;
	services.processWindowEvents = nullptr;
	services.setCurrentWindow = nullptr;
	services.getCurrentWindow = nullptr;
	services.getFocusedWindow = nullptr;
	services.getHoveredWindow = nullptr;
	services.createWindow = nullptr;
	services.setWindowTitle = nullptr;
	services.getWindowDisplayIndex = nullptr;
	services.getDisplayCount = nullptr;
	services.getDisplayInfo = nullptr;
	services.setWindowSize = nullptr;
	services.getWindowSize = nullptr;
	services.setWindowPosition = nullptr;
	services.getWindowPosition = nullptr;
	services.getWindowState = nullptr;
	services.presentWindow = nullptr;
	services.destroyWindow = nullptr;
	services.showWindow = nullptr;
	services.hideWindow = nullptr;
	services.raiseWindow = nullptr;
	services.maximizeWindow = nullptr;
	services.minimizeWindow = nullptr;
	services.setCapture = nullptr;
	services.releaseCapture = nullptr;
	services.getAbsoluteMousePosition = nullptr;
	services.isMouseButtonDownNow = nullptr;
	services.setCursor = nullptr;
	services.createCustomCursor = nullptr;
	services.deleteCustomCursor = nullptr;
	services.setCustomCursor = nullptr;
}

}
