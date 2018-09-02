#include "sdl2_input_provider.h"
#include <string.h>
#include <algorithm>

namespace hui
{
Sdl2InputProvider::Sdl2InputProvider()
{
	for (int i = 0; i < SDL_NUM_SYSTEM_CURSORS; i++)
	{
		cursors[i] = SDL_CreateSystemCursor((SDL_SystemCursor)i);
	}

	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
}

Sdl2InputProvider::~Sdl2InputProvider()
{
	for (size_t i = 0; i < customCursors.size(); i++)
	{
		SDL_FreeCursor(customCursors[i]);
		SDL_FreeSurface(customCursorSurfaces[i]);
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
	case SDLK_QUOTEDBL: key = KeyCode::None; break;
	case SDLK_HASH: key = KeyCode::None; break;
	case SDLK_PERCENT: key = KeyCode::None; break;
	case SDLK_DOLLAR: key = KeyCode::None; break;
	case SDLK_AMPERSAND: key = KeyCode::None; break;
	case SDLK_QUOTE: key = KeyCode::None; break;
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
	case SDLK_BACKQUOTE: key = KeyCode::None; break;
	case SDLK_a: key = KeyCode::A; break;
	case SDLK_b: key = KeyCode::B; break;
	case SDLK_c: key = KeyCode::C; break;
	case SDLK_d: key = KeyCode::D; break;
	case SDLK_e: key = KeyCode::E; break;
	case SDLK_f: key = KeyCode::F; break;
	case SDLK_g: key = KeyCode::G; break;
	case SDLK_h: key = KeyCode::H; break;
	case SDLK_i: key = KeyCode::I; break;
	case SDLK_j: key = KeyCode::J; break;
	case SDLK_k: key = KeyCode::K; break;
	case SDLK_l: key = KeyCode::L; break;
	case SDLK_m: key = KeyCode::M; break;
	case SDLK_n: key = KeyCode::N; break;
	case SDLK_o: key = KeyCode::O; break;
	case SDLK_p: key = KeyCode::P; break;
	case SDLK_q: key = KeyCode::Q; break;
	case SDLK_r: key = KeyCode::R; break;
	case SDLK_s: key = KeyCode::S; break;
	case SDLK_t: key = KeyCode::T; break;
	case SDLK_u: key = KeyCode::U; break;
	case SDLK_v: key = KeyCode::V; break;
	case SDLK_w: key = KeyCode::W; break;
	case SDLK_x: key = KeyCode::X; break;
	case SDLK_y: key = KeyCode::Y; break;
	case SDLK_z: key = KeyCode::Z; break;
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
	case SDLK_AUDIONEXT: key = KeyCode::None; break;
	case SDLK_AUDIOPREV: key = KeyCode::None; break;
	case SDLK_AUDIOSTOP: key = KeyCode::None; break;
	case SDLK_AUDIOPLAY: key = KeyCode::None; break;
	case SDLK_AUDIOMUTE: key = KeyCode::None; break;
	case SDLK_MEDIASELECT: key = KeyCode::None; break;
	case SDLK_WWW: key = KeyCode::None; break;
	case SDLK_MAIL: key = KeyCode::None; break;
	case SDLK_CALCULATOR: key = KeyCode::None; break;
	case SDLK_COMPUTER: key = KeyCode::None; break;
	case SDLK_AC_SEARCH: key = KeyCode::None; break;
	case SDLK_AC_HOME: key = KeyCode::None; break;
	case SDLK_AC_BACK: key = KeyCode::None; break;
	case SDLK_AC_FORWARD: key = KeyCode::None; break;
	case SDLK_AC_STOP: key = KeyCode::None; break;
	case SDLK_AC_REFRESH: key = KeyCode::None; break;
	case SDLK_AC_BOOKMARKS: key = KeyCode::None; break;
	case SDLK_BRIGHTNESSDOWN: key = KeyCode::None; break;
	case SDLK_BRIGHTNESSUP: key = KeyCode::None; break;
	case SDLK_DISPLAYSWITCH: key = KeyCode::None; break;
	case SDLK_KBDILLUMTOGGLE: key = KeyCode::None; break;
	case SDLK_KBDILLUMDOWN: key = KeyCode::None; break;
	case SDLK_KBDILLUMUP: key = KeyCode::None; break;
	case SDLK_EJECT: key = KeyCode::None; break;
	case SDLK_SLEEP: key = KeyCode::None; break;
	default:
		break;
	}

	return key;
}

void Sdl2InputProvider::startTextInput(Window window, const Rect& imeRect)
{
	SDL_Rect rc;

	rc.x = imeRect.x;
	rc.y = imeRect.y;
	rc.w = imeRect.width;
	rc.h = imeRect.height;

	SDL_SetTextInputRect(&rc);
	SDL_StartTextInput();
}

void Sdl2InputProvider::stopTextInput()
{
	SDL_StopTextInput();
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

	memcpy(outText, txt, std::min(strlen(txt) + 1, maxTextSize));
	SDL_free(txt);

	return true;
}

void Sdl2InputProvider::addSdlEvent(SDL_Event& ev)
{
    InputEvent outEvent = InputEvent();

    switch (ev.type)
    {
    case SDL_MOUSEMOTION:
	if (!addedMouseMove)
    {
		addedMouseMove = true;
		hui::setMouseMoved(true);
		outEvent.type = InputEvent::Type::MouseMove;
		outEvent.mouse.point.x = ev.motion.x;
		outEvent.mouse.point.y = ev.motion.y;
		outEvent.window = SDL_GetWindowFromID(ev.motion.windowID);
		focusedWindow = (SDL_Window*)outEvent.window;
	}
    else
    {
        return;
    }
        break;
    case SDL_MOUSEBUTTONDOWN:
        outEvent.type = InputEvent::Type::MouseDown;
        outEvent.mouse.point.x = ev.button.x;
        outEvent.mouse.point.y = ev.button.y;
        outEvent.mouse.button = (MouseButton)(ev.button.button - 1);
        outEvent.mouse.clickCount = ev.button.clicks;
        outEvent.window = SDL_GetWindowFromID(ev.button.windowID);
        focusedWindow = (SDL_Window*)outEvent.window;
        break;
    case SDL_MOUSEBUTTONUP:
        outEvent.type = InputEvent::Type::MouseUp;
        outEvent.mouse.point.x = ev.button.x;
        outEvent.mouse.point.y = ev.button.y;
        outEvent.mouse.button = (MouseButton)(ev.button.button - 1);
        outEvent.mouse.clickCount = ev.button.clicks;
        outEvent.window = SDL_GetWindowFromID(ev.button.windowID);
        break;
    case SDL_MOUSEWHEEL:
        outEvent.type = InputEvent::Type::MouseWheel;
        int x, y;
        SDL_GetMouseState(&x, &y);
        outEvent.mouse.point.x = x;
        outEvent.mouse.point.y = y;
        outEvent.mouse.button = (MouseButton)(ev.button.button - 1);
        outEvent.mouse.clickCount = ev.button.clicks;
        outEvent.mouse.wheel.x = ev.wheel.x;
        outEvent.mouse.wheel.y = ev.wheel.y;
        outEvent.window = SDL_GetWindowFromID(ev.wheel.windowID);
        break;
    case SDL_KEYDOWN:
        outEvent.type = InputEvent::Type::Key;
        outEvent.key.down = true;
        outEvent.key.code = fromSdlKey(ev.key.keysym.sym);
        outEvent.window = SDL_GetWindowFromID(ev.key.windowID);
	outEvent.key.modifiers = KeyModifiers::None;
	outEvent.key.modifiers |= (ev.key.keysym.mod & KMOD_ALT) ? KeyModifiers::Alt : KeyModifiers::None;
    	outEvent.key.modifiers |= (ev.key.keysym.mod & KMOD_SHIFT) ? KeyModifiers::Shift : KeyModifiers::None;
    	outEvent.key.modifiers |= (ev.key.keysym.mod & KMOD_CTRL) ? KeyModifiers::Control : KeyModifiers::None;
        break;
    case SDL_KEYUP:
        outEvent.type = InputEvent::Type::Key;
        outEvent.key.down = false;
        outEvent.key.code = fromSdlKey(ev.key.keysym.sym);
        outEvent.window = SDL_GetWindowFromID(ev.key.windowID);
	outEvent.key.modifiers = KeyModifiers::None;
	outEvent.key.modifiers |= (ev.key.keysym.mod & KMOD_ALT) ? KeyModifiers::Alt : KeyModifiers::None;
    	outEvent.key.modifiers |= (ev.key.keysym.mod & KMOD_SHIFT) ? KeyModifiers::Shift : KeyModifiers::None;
    	outEvent.key.modifiers |= (ev.key.keysym.mod & KMOD_CTRL) ? KeyModifiers::Control : KeyModifiers::None;
        break;
    case SDL_TEXTINPUT:
        outEvent.type = InputEvent::Type::Text;
        strcpy(outEvent.text.text, ev.text.text);
        outEvent.window = SDL_GetWindowFromID(ev.text.windowID);
	outEvent.key.modifiers = KeyModifiers::None;
        break;
    case SDL_DROPFILE:
        outEvent.type = InputEvent::Type::OsDragDrop;
        outEvent.drop.type = InputEvent::OsDragDropData::Type::DropFile;
        outEvent.drop.filename = new char[strlen(ev.drop.file) + 1];
        strcpy(outEvent.drop.filename, ev.drop.file);
        SDL_free(ev.drop.file);
        outEvent.window = SDL_GetWindowFromID(ev.drop.windowID);
        break;
    case SDL_DROPTEXT:
        outEvent.type = InputEvent::Type::OsDragDrop;
        outEvent.drop.type = InputEvent::OsDragDropData::Type::DropText;
        outEvent.drop.filename = new char[strlen(ev.drop.file) + 1];
        strcpy(outEvent.drop.filename, ev.drop.file);
        SDL_free(ev.drop.file);
        outEvent.window = SDL_GetWindowFromID(ev.drop.windowID);
        break;
    case SDL_DROPBEGIN:
        outEvent.type = InputEvent::Type::OsDragDrop;
        outEvent.drop.type = InputEvent::OsDragDropData::Type::DropBegin;
        outEvent.window = SDL_GetWindowFromID(ev.drop.windowID);
        break;
    case SDL_DROPCOMPLETE:
        outEvent.type = InputEvent::Type::OsDragDrop;
        outEvent.drop.type = InputEvent::OsDragDropData::Type::DropComplete;
        outEvent.window = SDL_GetWindowFromID(ev.drop.windowID);
        break;
    case SDL_WINDOWEVENT:
    {
        switch (ev.window.event)
        {
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        case SDL_WINDOWEVENT_MOVED:
        case SDL_WINDOWEVENT_EXPOSED:
            outEvent.type = InputEvent::Type::WindowResize;
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
        {
            outEvent.type = InputEvent::Type::WindowGotFocus;
            focusedWindow = SDL_GetWindowFromID(ev.window.windowID);
            break;
        }
        case SDL_WINDOWEVENT_ENTER:
        {
            outEvent.type = InputEvent::Type::WindowGotFocus;
            hoveredWindow = SDL_GetWindowFromID(ev.window.windowID);
            break;
        }
        case SDL_WINDOWEVENT_FOCUS_LOST:
            outEvent.type = InputEvent::Type::WindowLostFocus;
            break;
        case SDL_WINDOWEVENT_CLOSE:
        {
            outEvent.type = InputEvent::Type::WindowClose;
            break;
        }
        default:
            break;
        }

        outEvent.window = SDL_GetWindowFromID(ev.window.windowID);

        break;
    }
    default:
        break;
    }

    if (ev.type == SDL_KEYDOWN
		|| ev.type == SDL_KEYUP
		|| ev.type == SDL_MOUSEBUTTONDOWN
		|| ev.type == SDL_MOUSEBUTTONUP
		|| ev.type == SDL_MOUSEWHEEL
		|| ev.type == SDL_TEXTINPUT
		|| ev.type == SDL_TEXTEDITING
		|| ev.type == SDL_WINDOWEVENT
		|| ev.type == SDL_DROPFILE
		|| ev.type == SDL_DROPTEXT
		|| ev.type == SDL_DROPBEGIN
		|| ev.type == SDL_DROPCOMPLETE)
	{
		// do not add all the resize event, we just need one
		if (ev.type == SDL_WINDOWEVENT
			&& (ev.window.event == SDL_WINDOWEVENT_RESIZED
				|| ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED
				|| ev.window.event == SDL_WINDOWEVENT_MOVED
				|| ev.window.event == SDL_WINDOWEVENT_EXPOSED)
			&& sizeChanged)
		{
			return;
		}

		sizeChanged = true;
	}

	if ((ev.type == SDL_QUIT || ev.window.event == SDL_WINDOWEVENT_CLOSE)
		&& focusedWindow == mainWindow)
	{
		// give a chance the user to cancel quit, can be cancelled with cancelQuit()
		wantsToQuitApp = true;
	}

    addInputEvent(outEvent);
}

void Sdl2InputProvider::processSdlEvents()
{
	SDL_Event ev;
	sizeChanged = false;
	addedMouseMove = false;
    while (SDL_PollEvent(&ev))
	{
		addSdlEvent(ev);
	}
}

void Sdl2InputProvider::updateDeltaTime()
{
	u32 ticks = SDL_GetTicks();
	deltaTime = (f32)(ticks - lastTime) / 1000.0f;
	lastTime = ticks;
}

bool Sdl2InputProvider::mustQuit()
{
	return quitApp;
}

bool Sdl2InputProvider::wantsToQuit()
{
	return wantsToQuitApp;
}

void Sdl2InputProvider::cancelQuitApplication()
{
	wantsToQuitApp = false;
	quitApp = false;
	forceRepaint();
}

void Sdl2InputProvider::quitApplication()
{
	wantsToQuitApp = true;
	quitApp = true;
}

void Sdl2InputProvider::processEvents()
{
    updateDeltaTime();
    setFrameDeltaTime(deltaTime);
	processSdlEvents();
}

void Sdl2InputProvider::shutdown()
{
	SDL_Quit();
}

void Sdl2InputProvider::setCursor(MouseCursorType type)
{
	SDL_SetCursor(cursors[(int)type]);
}

MouseCursor Sdl2InputProvider::createCustomCursor(Rgba32* pixels, u32 width, u32 height, u32 hotX, u32 hotY)
{
	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormatFrom(
		pixels, width, height, 1, width * 4, SDL_PIXELFORMAT_RGBA32);

	auto cur = SDL_CreateColorCursor(surf, hotX, hotY);

	customCursors.push_back(cur);
	customCursorSurfaces.push_back(surf);

	return cur;
}

void Sdl2InputProvider::destroyCustomCursor(MouseCursor cursor)
{
	for (size_t i = 0; i < customCursors.size(); i++)
	{
		if (customCursors[i] == cursor)
		{
			SDL_FreeCursor(customCursors[i]);
			SDL_FreeSurface(customCursorSurfaces[i]);
			customCursors.erase(customCursors.begin() + i);
			customCursorSurfaces.erase(customCursorSurfaces.begin() + i);
			break;
		}
	}
}

void Sdl2InputProvider::setCustomCursor(MouseCursor cursor)
{
	SDL_SetCursor((SDL_Cursor*)cursor);
}

Window Sdl2InputProvider::getMainWindow()
{
	return (Window)mainWindow;
}

void Sdl2InputProvider::setCurrentWindow(Window window)
{
    if (gfxProvider->getApiType() == GraphicsProvider::ApiType::OpenGL)
    {
        SDL_GL_MakeCurrent((SDL_Window*)window, sdlOpenGLCtx);
    }

	currentWindow = (SDL_Window*)window;
}

Window Sdl2InputProvider::getCurrentWindow()
{
	return currentWindow;
}

Window Sdl2InputProvider::getFocusedWindow()
{
	return focusedWindow;
}

Window Sdl2InputProvider::getHoveredWindow()
{
	return hoveredWindow;
}

Window Sdl2InputProvider::createWindow(
	const char* title, i32 width, i32 height,
	WindowBorder border, WindowPositionType windowPos,
	Point customPosition, bool showInTaskBar)
{
	int posx = SDL_WINDOWPOS_UNDEFINED, posy = SDL_WINDOWPOS_UNDEFINED;

	if (windowPos == WindowPositionType::Centered)
	{
		posx = posy = SDL_WINDOWPOS_CENTERED;
	}
	else if (windowPos == WindowPositionType::Custom)
	{
		posx = customPosition.x;
		posy = customPosition.y;
	}

	int flags = SDL_WINDOW_SHOWN;

	switch (border)
	{
	case WindowBorder::Resizable:
		flags |= SDL_WINDOW_RESIZABLE;
		break;
	case WindowBorder::Fixed:
		break;
	case WindowBorder::ResizableNoTitle:
		flags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS;
		break;
	case WindowBorder::FixedNoTitle:
		flags |= SDL_WINDOW_BORDERLESS;
		break;
	default:
		break;
	}

	auto wnd = SDL_CreateWindow(
		title, posx, posy, width, height,
		SDL_WINDOW_OPENGL | flags);

	return wnd;
}

void Sdl2InputProvider::setWindowTitle(Window window, const char* title)
{
	SDL_SetWindowTitle((SDL_Window*)window, title);
}

void Sdl2InputProvider::setWindowRect(Window window, const Rect& rect)
{
	SDL_SetWindowPosition((SDL_Window*)window, rect.x, rect.y);
	SDL_SetWindowSize((SDL_Window*)window, rect.width, rect.height);
}

Rect Sdl2InputProvider::getWindowRect(Window window)
{
	SDL_Rect rc;

	SDL_GetWindowPosition((SDL_Window*)window, &rc.x, &rc.y);
	SDL_GetWindowSize((SDL_Window*)window, &rc.w, &rc.h);

	return{ (f32)rc.x, (f32)rc.y, (f32)rc.w, (f32)rc.h };
}

void Sdl2InputProvider::presentWindow(Window window)
{
	SDL_GL_SwapWindow((SDL_Window*)window);
}

void Sdl2InputProvider::destroyWindow(Window window)
{
	SDL_DestroyWindow((SDL_Window*)window);
}

void Sdl2InputProvider::showWindow(Window window)
{
	SDL_ShowWindow((SDL_Window*)window);
}

void Sdl2InputProvider::hideWindow(Window window)
{
	SDL_HideWindow((SDL_Window*)window);
}

void Sdl2InputProvider::raiseWindow(Window window)
{
	SDL_RaiseWindow((SDL_Window*)window);
}

void Sdl2InputProvider::maximizeWindow(Window window)
{
	SDL_MaximizeWindow((SDL_Window*)window);
}

void Sdl2InputProvider::minimizeWindow(Window window)
{
	SDL_MinimizeWindow((SDL_Window*)window);
}

WindowState Sdl2InputProvider::getWindowState(Window window)
{
	auto flags = SDL_GetWindowFlags((SDL_Window*)window);

	if (flags & SDL_WINDOW_HIDDEN)
	{
		return WindowState::Hidden;
	}

	if (flags & SDL_WINDOW_MINIMIZED)
	{
		return WindowState::Minimized;
	}

	if (flags & SDL_WINDOW_MAXIMIZED)
	{
		return WindowState::Maximized;
	}

	return WindowState::Normal;
}

void Sdl2InputProvider::setCapture(Window window)
{
	SDL_CaptureMouse(SDL_TRUE);
}

void Sdl2InputProvider::releaseCapture()
{
	SDL_CaptureMouse(SDL_FALSE);
}

Point Sdl2InputProvider::getMousePosition()
{
	int x, y;

	SDL_GetMouseState(&x, &y);

	return { (f32)x , (f32)y };
}

void initializeWithSDL(const SdlSettings& settings)
{
	Rect wndRect = settings.mainWindowRect;

    if (!settings.gfxProvider)
    {
        printf("SDL needs a graphics provider\n");
        return;
    }

	printf("Initializing SDL...\n");

    if (!settings.sdlContext)
    {
        SDL_SetMainReady();

        int err = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

        if (err != 0)
        {
            printf("SDL initialize error: %s\n", SDL_GetError());
            return;
        }
    }

    if (settings.gfxProvider->getApiType() == GraphicsProvider::ApiType::OpenGL)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    }

	Sdl2InputProvider* inputProvider = new Sdl2InputProvider();
    
    inputProvider->gfxProvider = settings.gfxProvider;
    
    bool maximize = false;
    SDL_Window* wnd = nullptr;

    inputProvider->context = createContext(inputProvider, settings.gfxProvider);

    if (!settings.sdlMainWindow)
    {
        if (settings.mainWindowRect.isZero())
        {
            wndRect.set(0, 0, 1024, 768);
            maximize = true;
        }
        else
        {
            wndRect = settings.mainWindowRect;
        }
        
        wnd = (SDL_Window*)inputProvider->createWindow(
            settings.mainWindowTitle,
            (int)wndRect.width,
            (int)wndRect.height,
            WindowBorder::Resizable,
            settings.positionType,
            { wndRect.x, wndRect.y },
            true);

        if (!wnd)
        {
            printf("Cannot create SDL window: %s: %s\n", settings.mainWindowTitle, SDL_GetError());
            return;
        }

        if (inputProvider->gfxProvider->getApiType() == GraphicsProvider::ApiType::OpenGL)
        {
            inputProvider->sdlOpenGLCtx = SDL_GL_CreateContext(wnd);

            if (!inputProvider->sdlOpenGLCtx)
            {
                printf("Cannot create GL context for SDL: %s\n", SDL_GetError());
            }
        }
    }
    else
    {
        wnd = settings.sdlMainWindow;
    }

    if (settings.gfxProvider->getApiType() == GraphicsProvider::ApiType::OpenGL)
    {
        SDL_GL_MakeCurrent(wnd, inputProvider->sdlOpenGLCtx);
        SDL_GL_SetSwapInterval(settings.vSync ? 1 : 0);
    }

    settings.gfxProvider->initialize();
    initializeContext(inputProvider->context);
    inputProvider->mainWindow = wnd;
	inputProvider->focusedWindow = wnd;
	
    SDL_ShowWindow(wnd);
	SDL_RaiseWindow(wnd);

	if (maximize)
	{
		SDL_MaximizeWindow(wnd);
	}
}

}
