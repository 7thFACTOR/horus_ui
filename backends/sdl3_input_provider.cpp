#define NOMINMAX
#include "sdl3_input_provider.h"
#include <assert.h>
#include <string.h>
#include <SDL3/SDL_main.h>
#include <glad/gl.h>
#ifdef _WINDOWS
#include <windows.h>
#endif
#include <algorithm>

namespace hui
{
SDL_HitTestResult HitTestCallbackForResize(SDL_Window *Window, const SDL_Point *Area, void *Data)
{
    int Width, Height;
    SDL_GetWindowSize(Window, &Width, &Height);
	const int MOUSE_GRAB_PADDING = 3;

    if(Area->y < MOUSE_GRAB_PADDING)
    {
        if(Area->x < MOUSE_GRAB_PADDING)
        {
            return SDL_HITTEST_RESIZE_TOPLEFT;
        }
        else if(Area->x > Width - MOUSE_GRAB_PADDING)
        {
            return SDL_HITTEST_RESIZE_TOPRIGHT;
        }
        else
        {
            return SDL_HITTEST_RESIZE_TOP;
        }
    }
    else if(Area->y > Height - MOUSE_GRAB_PADDING)
    {
        if(Area->x < MOUSE_GRAB_PADDING)
        {
            return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        }
        else if(Area->x > Width - MOUSE_GRAB_PADDING)
        {
            return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
        }
        else
        {
            return SDL_HITTEST_RESIZE_BOTTOM;
        }
    }
    else if(Area->x < MOUSE_GRAB_PADDING)
    {
        return SDL_HITTEST_RESIZE_LEFT;
    }
    else if(Area->x > Width - MOUSE_GRAB_PADDING)
    {
        return SDL_HITTEST_RESIZE_RIGHT;
    }
    //else if(Area->y < 70)
    //{
    //    return SDL_HITTEST_DRAGGABLE;
    //}

    return SDL_HITTEST_NORMAL; //SDL_HITTEST_DRAGGABLE; // SDL_HITTEST_NORMAL <- Windows behaviour
}

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

static void removeWindowShadow_Windows(SDL_Window* window) {
	HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	if (hwnd) {
		LONG_PTR style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
		style &= ~WS_EX_COMPOSITED; // Remove composition (shadow)
		style &= ~WS_EX_APPWINDOW;
		style |= WS_EX_TOOLWINDOW;
		SetWindowLongPtr(hwnd, GWL_EXSTYLE, style);
		SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
}
#endif

Sdl2InputProvider::Sdl2InputProvider()
{}

Sdl2InputProvider::~Sdl2InputProvider()
{
	for (size_t i = 0; i < customCursors.size(); i++)
	{
		SDL_DestroyCursor(customCursors[i]);
		SDL_DestroySurface(customCursorSurfaces[i]);
	}
}

KeyCode Sdl2InputProvider::fromSdlKey(int code)
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

void Sdl2InputProvider::startTextInput(HOsWindow window, const Rect& imeRect)
{
	SDL_Rect rc = {0};

	rc.x = imeRect.x;
	rc.y = imeRect.y;
	rc.w = imeRect.width;
	rc.h = imeRect.height;	
	
	if (window)
	{
		textInputWindow = ((SdlWindowProxy*)window)->sdlWindow;
		SDL_SetTextInputArea(textInputWindow, &rc, 0);
		SDL_StartTextInput(textInputWindow);
	}
}

void Sdl2InputProvider::stopTextInput()
{
	SDL_StopTextInput(textInputWindow);
}

bool Sdl2InputProvider::copyToClipboard(const char* text)
{
	return 0 == SDL_SetClipboardText(text);
}

bool Sdl2InputProvider::pasteFromClipboard(char* outText, u32 maxTextSize)
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

void Sdl2InputProvider::addSdlEvent(SDL_Event& ev)
{
	InputEvent outEvent = InputEvent();

	switch (ev.type)
	{
	case SDL_EVENT_MOUSE_MOTION:
		if (!addedMouseMove)
		{
			addedMouseMove = true;
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
			focusedWindow = (SdlWindowProxy*)outEvent.window;
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
		focusedWindow = (SdlWindowProxy*)outEvent.window;
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
		outEvent.type = InputEvent::Type::WindowResized;
		break;
	case SDL_EVENT_WINDOW_FOCUS_GAINED:
	{
		outEvent.type = InputEvent::Type::WindowGotFocus;
		focusedWindow = findSdlWindow(SDL_GetWindowFromID(ev.window.windowID));
		break;
	}
	case SDL_EVENT_WINDOW_MOUSE_ENTER:
	{
		outEvent.type = InputEvent::Type::WindowMouseEnter;
		hoveredWindow = findSdlWindow(SDL_GetWindowFromID(ev.window.windowID));
		break;
	}
	case SDL_EVENT_WINDOW_MOUSE_LEAVE:
	{
		outEvent.type = InputEvent::Type::WindowMouseLeave;
		hoveredWindow = nullptr;
		break;
	}
	case SDL_EVENT_WINDOW_FOCUS_LOST:
		outEvent.type = InputEvent::Type::WindowLostFocus;
		focusedWindow = nullptr;
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

void Sdl2InputProvider::processSdlEvents()
{
	SDL_Event ev;

	addedMouseMove = false;

	while (SDL_PollEvent(&ev))
	{
		addSdlEvent(ev);
	}
}

SdlWindowProxy* Sdl2InputProvider::findSdlWindow(SDL_Window* wnd)
{
	for (auto& w : windows)
	{
		if (w->sdlWindow == wnd)
			return w;
	}

	return nullptr;
}

void Sdl2InputProvider::updateDeltaTime()
{
	u32 ticks = SDL_GetTicks();
	deltaTime = (f32)(ticks - lastTime) / 1000.0f;
	lastTime = ticks;
}

void Sdl2InputProvider::processEvents()
{
	updateDeltaTime();
	setFrameDeltaTime(deltaTime);
	processSdlEvents();
}

void Sdl2InputProvider::shutdown()
{
	if (ownsGlContext)
		SDL_GL_DestroyContext(initParams.sdlGlContext);

	if (ownsSdlInit)
		SDL_Quit();
}

void Sdl2InputProvider::setCursor(MouseCursorType type)
{
	SDL_SetCursor(cursors[(int)type]);
}

HMouseCursor Sdl2InputProvider::createCustomCursor(Rgba32* pixels, u32 width, u32 height, u32 hotX, u32 hotY)
{
	SDL_Surface* surf = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * 4);

	auto cur = SDL_CreateColorCursor(surf, hotX, hotY);

	customCursors.push_back(cur);
	customCursorSurfaces.push_back(surf);

	return cur;
}

void Sdl2InputProvider::deleteCustomCursor(HMouseCursor cursor)
{
	for (size_t i = 0; i < customCursors.size(); i++)
	{
		if (customCursors[i] == cursor)
		{
			SDL_DestroyCursor(customCursors[i]);
			SDL_DestroySurface(customCursorSurfaces[i]);
			customCursors.erase(customCursors.begin() + i);
			customCursorSurfaces.erase(customCursorSurfaces.begin() + i);
			break;
		}
	}
}

void Sdl2InputProvider::setCustomCursor(HMouseCursor cursor)
{
	SDL_SetCursor((SDL_Cursor*)cursor);
}

void Sdl2InputProvider::setCurrentWindow(HOsWindow window)
{
	if (HORUS_GFX->getApiType() == GraphicsProvider::ApiType::OpenGL)
	{
		SDL_GL_MakeCurrent(((SdlWindowProxy*)window)->sdlWindow, initParams.sdlGlContext);
		SDL_GL_SetSwapInterval(initParams.vSync ? 1 : 0);
	}

	currentWindow = ((SdlWindowProxy*)window);
}

HOsWindow Sdl2InputProvider::getCurrentWindow()
{
	return currentWindow;
}

HOsWindow Sdl2InputProvider::getFocusedWindow()
{
	return focusedWindow;
}

HOsWindow Sdl2InputProvider::getHoveredWindow()
{
	return hoveredWindow;
}

// Hit-test callback that makes the window transparent to mouse events
SDL_HitTestResult HitTestCallback(SDL_Window* win, const SDL_Point* area, void* data) {
	return SDL_HITTEST_NORMAL; // Ignore input, pass through to windows underneath
}

HOsWindow Sdl2InputProvider::createWindow(
	const char* title, OsWindowFlags flags, OsWindowState state, const Rect& rect)
{
	int sdlflags = 0;

	if (state == OsWindowState::Maximized)
		sdlflags |= SDL_WINDOW_MAXIMIZED;

	if (state == OsWindowState::Minimized)
		sdlflags |= SDL_WINDOW_MINIMIZED;

	if (state == OsWindowState::Hidden)
		sdlflags |= SDL_WINDOW_HIDDEN;

	if (has(flags, OsWindowFlags::Resizable))
		sdlflags |= SDL_WINDOW_RESIZABLE;

	if (has(flags, OsWindowFlags::NoDecoration))
		sdlflags |= SDL_WINDOW_BORDERLESS;

	if (has(flags, OsWindowFlags::NoTaskBar))
		sdlflags |= SDL_WINDOW_UTILITY;

	if (HORUS_GFX->getApiType() == GraphicsProvider::ApiType::OpenGL)
		sdlflags |= SDL_WINDOW_OPENGL;

	if (HORUS_GFX->getApiType() == GraphicsProvider::ApiType::Vulkan)
		sdlflags |= SDL_WINDOW_VULKAN;

	if (HORUS_GFX->getApiType() == GraphicsProvider::ApiType::Metal)
		sdlflags |= SDL_WINDOW_METAL;

	//if (HORUS_GFX->getApiType() == GraphicsProvider::ApiType::Direct3D11)
	//	sdlflags |= SDL_WINDOW_DIRECTX;

	//if (HORUS_GFX->getApiType() == GraphicsProvider::ApiType::Direct3D12)
	//	sdlflags |= SDL_WINDOW_DX12;

	auto wnd = SDL_CreateWindow(
		title, rect.width, rect.height,
		sdlflags);

	auto newWnd = new SdlWindowProxy();

	newWnd->sdlWindow = wnd;
	windows.push_back(newWnd);

	focusedWindow = newWnd;
	currentWindow = newWnd;

	if (has(flags, OsWindowFlags::NoInput))
	{
#ifdef _WINDOWS
		makeWindowClickThrough_Windows(wnd);
		//removeWindowShadow_Windows(wnd);
#endif
		
#ifdef _LINUX
		makeWindowClickThrough_Linux(wnd);
#endif
	}

	// if no GL context provided, create one for this first window
	if (HORUS_GFX->getApiType() == GraphicsProvider::ApiType::OpenGL && !initParams.sdlGlContext)
	{
		initParams.sdlGlContext = SDL_GL_CreateContext(wnd);
		ownsGlContext = true;

		if (!initParams.sdlGlContext)
		{
			printf("Cannot create GL context for SDL: %s\n", SDL_GetError());
		}

		SDL_GL_MakeCurrent(wnd, initParams.sdlGlContext);
		SDL_GL_SetSwapInterval(initParams.vSync ? 1 : 0);

		if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
		{
			printf("GLAD cannot init GL func ptrs\n");
		}

		const GLubyte* renderer = glGetString(GL_RENDERER);  // Get renderer string
		const GLubyte* version = glGetString(GL_VERSION);    // Get version string
		printf("GL Renderer: %s\n", renderer);
		printf("GL Version: %s\n", version);
	}

	SDL_SetWindowPosition(wnd, rect.x, rect.y);
	SDL_SyncWindow(wnd);
	SDL_RaiseWindow(wnd);

	return newWnd;
}

void Sdl2InputProvider::setWindowTitle(HOsWindow window, const char* title)
{
	SDL_SetWindowTitle(((SdlWindowProxy*)window)->sdlWindow, title);
}

std::string Sdl2InputProvider::getWindowTitle(HOsWindow window)
{
	return SDL_GetWindowTitle(((SdlWindowProxy*)window)->sdlWindow);
}

u32 Sdl2InputProvider::getWindowDisplayIndex(HOsWindow window)
{
	return SDL_GetDisplayForWindow(((SdlWindowProxy*)window)->sdlWindow);
}

u32 Sdl2InputProvider::getDisplayCount() const
{
	i32 count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&count);
	
	SDL_free(displays);

	return count;
}

DisplayInfo Sdl2InputProvider::getDisplayInfo(u32 displayIndex)
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

void Sdl2InputProvider::setWindowClientSize(HOsWindow window, const Point& size)
{
	SDL_SetWindowSize(((SdlWindowProxy*)window)->sdlWindow, size.x, size.y);
}

Point Sdl2InputProvider::getWindowClientSize(HOsWindow window)
{
	int w = 0, h = 0;

	SDL_GetWindowSize(((SdlWindowProxy*)window)->sdlWindow, &w, &h);

	return { (f32)w, (f32)h };
}

Rect Sdl2InputProvider::getWindowRect(HOsWindow window)
{
	int top = 0, left = 0, right = 0, bottom = 0;
	int x = 0, y = 0;
	int w = 0, h = 0;

	SDL_GetWindowPosition(((SdlWindowProxy*)window)->sdlWindow, &x, &y);
	SDL_GetWindowSize(((SdlWindowProxy*)window)->sdlWindow, &w, &h);

	return { (f32)x, (f32)y, (f32)(w), (f32)(h) };
}

void Sdl2InputProvider::setWindowRect(HOsWindow window, const Rect& rect)
{
	SDL_SetWindowPosition(((SdlWindowProxy*)window)->sdlWindow, rect.x, rect.y);
	SDL_SetWindowSize(((SdlWindowProxy*)window)->sdlWindow, rect.width, rect.height);
}

void Sdl2InputProvider::setWindowPosition(HOsWindow window, const Point& pos)
{
	SDL_SetWindowPosition(((SdlWindowProxy*)window)->sdlWindow, pos.x, pos.y);
}

Point Sdl2InputProvider::getWindowPosition(HOsWindow window)
{
	int x = 0, y = 0;

	SDL_GetWindowPosition(((SdlWindowProxy*)window)->sdlWindow, &x, &y);

	return { (f32)x, (f32)y };
}

void Sdl2InputProvider::presentWindow(HOsWindow window)
{
	if (HORUS_GFX->getApiType() == GraphicsProvider::ApiType::OpenGL)
	{
		SDL_GL_SwapWindow(((SdlWindowProxy*)window)->sdlWindow);
	}
}

void Sdl2InputProvider::destroyWindow(HOsWindow window)
{
	SDL_DestroyWindow(((SdlWindowProxy*)window)->sdlWindow);
	auto iter = std::find(windows.begin(), windows.end(), window);

	if (iter != windows.end())
	{
		delete ((SdlWindowProxy*)window);
		windows.erase(iter);
	}
}

void Sdl2InputProvider::showWindow(HOsWindow window)
{
	SDL_ShowWindow(((SdlWindowProxy*)window)->sdlWindow);
}

void Sdl2InputProvider::hideWindow(HOsWindow window)
{
	SDL_HideWindow(((SdlWindowProxy*)window)->sdlWindow);
}

void Sdl2InputProvider::raiseWindow(HOsWindow window)
{
	SDL_RaiseWindow(((SdlWindowProxy*)window)->sdlWindow);
}

void Sdl2InputProvider::maximizeWindow(HOsWindow window)
{
	SDL_MaximizeWindow(((SdlWindowProxy*)window)->sdlWindow);
}

void Sdl2InputProvider::minimizeWindow(HOsWindow window)
{
	SDL_MinimizeWindow(((SdlWindowProxy*)window)->sdlWindow);
}

OsWindowState Sdl2InputProvider::getWindowState(HOsWindow window)
{
	auto flags = SDL_GetWindowFlags(((SdlWindowProxy*)window)->sdlWindow);

	if (flags & SDL_WINDOW_HIDDEN)
	{
		return OsWindowState::Hidden;
	}

	if (flags & SDL_WINDOW_MINIMIZED)
	{
		return OsWindowState::Minimized;
	}

	if (flags & SDL_WINDOW_MAXIMIZED)
	{
		return OsWindowState::Maximized;
	}

	return OsWindowState::Normal;
}

void Sdl2InputProvider::setCapture(HOsWindow window)
{
	SDL_CaptureMouse(true);
}

void Sdl2InputProvider::releaseCapture()
{
	SDL_CaptureMouse(false);
}

Point Sdl2InputProvider::getAbsoluteMousePosition()
{
	f32 x, y;

	SDL_GetGlobalMouseState(&x, &y);

	return { (f32)x , (f32)y };
}

bool Sdl2InputProvider::isMouseButtonDownNow(MouseButton button)
{
	f32 x = 0, y = 0;
	auto buttons = SDL_GetGlobalMouseState(&x, &y);

	if (button == MouseButton::Left) return buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT);
	if (button == MouseButton::Middle) return buttons & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE);
	if (button == MouseButton::Right) return buttons & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT);

	return false;
}

void Sdl2InputProvider::createSystemCursors()
{
	for (int i = 0; i < SDL_SYSTEM_CURSOR_COUNT; i++)
	{
		cursors[i] = SDL_CreateSystemCursor((SDL_SystemCursor)i);
	}

	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
}

void initializeSdl(const SdlInitParams& params)
{
	assert(getContextSettings().providers.gfx);
	auto sdlProvider = ((Sdl2InputProvider*)HORUS_INPUT);
	assert(sdlProvider);
	printf("Initializing SDL...\n");

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

		sdlProvider->ownsSdlInit = true;
	}

	if (HORUS_GFX->getApiType() == GraphicsProvider::ApiType::OpenGL)
	{
		if (params.antiAliasing != AntiAliasing::None)
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);

		switch (params.antiAliasing)
		{
		case AntiAliasing::MSAA4X:
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
			break;
		case AntiAliasing::MSAA8X:
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
			break;
		case AntiAliasing::MSAA16X:
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
			break;
		}

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	}

	sdlProvider->initParams = params;
	sdlProvider->createSystemCursors();	
	SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");
}

}
