#include "sfml_input_provider.h"
#include <string.h>

namespace hui
{
void SfmlInputProvider::processSfmlEvent(hui::Window window, sf::Event& ev)
{
	hui::InputEvent event;

	auto fromSfmlKey = [](sf::Keyboard::Key sfkey)
	{
		switch (sfkey)
		{
		case sf::Keyboard::Unknown:
			return hui::KeyCode::None;
		case sf::Keyboard::A:
			return hui::KeyCode::A;
		case sf::Keyboard::B:
			return hui::KeyCode::B;
		case sf::Keyboard::C:
			return hui::KeyCode::C;
		case sf::Keyboard::D:
			return hui::KeyCode::D;
		case sf::Keyboard::E:
			return hui::KeyCode::E;
		case sf::Keyboard::F:
			return hui::KeyCode::F;
		case sf::Keyboard::G:
			return hui::KeyCode::G;
		case sf::Keyboard::H:
			return hui::KeyCode::H;
		case sf::Keyboard::I:
			return hui::KeyCode::I;
		case sf::Keyboard::J:
			return hui::KeyCode::J;
		case sf::Keyboard::K:
			return hui::KeyCode::K;
		case sf::Keyboard::L:
			return hui::KeyCode::L;
		case sf::Keyboard::M:
			return hui::KeyCode::M;
		case sf::Keyboard::N:
			return hui::KeyCode::N;
		case sf::Keyboard::O:
			return hui::KeyCode::O;
		case sf::Keyboard::P:
			return hui::KeyCode::P;
		case sf::Keyboard::Q:
			return hui::KeyCode::Q;
		case sf::Keyboard::R:
			return hui::KeyCode::R;
		case sf::Keyboard::S:
			return hui::KeyCode::S;
		case sf::Keyboard::T:
			return hui::KeyCode::T;
		case sf::Keyboard::U:
			return hui::KeyCode::U;
		case sf::Keyboard::V:
			return hui::KeyCode::V;
		case sf::Keyboard::W:
			return hui::KeyCode::W;
		case sf::Keyboard::X:
			return hui::KeyCode::X;
		case sf::Keyboard::Y:
			return hui::KeyCode::Y;
		case sf::Keyboard::Z:
			return hui::KeyCode::Z;
		case sf::Keyboard::Num0:
			return hui::KeyCode::Num0;
		case sf::Keyboard::Num1:
			return hui::KeyCode::Num1;
		case sf::Keyboard::Num2:
			return hui::KeyCode::Num2;
		case sf::Keyboard::Num3:
			return hui::KeyCode::Num3;
		case sf::Keyboard::Num4:
			return hui::KeyCode::Num4;
		case sf::Keyboard::Num5:
			return hui::KeyCode::Num5;
		case sf::Keyboard::Num6:
			return hui::KeyCode::Num6;
		case sf::Keyboard::Num7:
			return hui::KeyCode::Num7;
		case sf::Keyboard::Num8:
			return hui::KeyCode::Num8;
		case sf::Keyboard::Num9:
			return hui::KeyCode::Num9;
		case sf::Keyboard::Escape:
			return hui::KeyCode::Esc;
		case sf::Keyboard::LControl:
			return hui::KeyCode::LControl;
		case sf::Keyboard::LShift:
			return hui::KeyCode::LShift;
		case sf::Keyboard::LAlt:
			return hui::KeyCode::LAlt;
		case sf::Keyboard::LSystem:
			return hui::KeyCode::LWin;
		case sf::Keyboard::RControl:
			return hui::KeyCode::RControl;
		case sf::Keyboard::RShift:
			return hui::KeyCode::RShift;
		case sf::Keyboard::RAlt:
			return hui::KeyCode::RAlt;
		case sf::Keyboard::RSystem:
			return hui::KeyCode::RWin;
		case sf::Keyboard::Menu:
			return hui::KeyCode::Apps;
		case sf::Keyboard::LBracket:
			return hui::KeyCode::LBracket;
		case sf::Keyboard::RBracket:
			return hui::KeyCode::RBracket;
		case sf::Keyboard::Semicolon:
			return hui::KeyCode::Semicolon;
		case sf::Keyboard::Comma:
			return hui::KeyCode::Comma;
		case sf::Keyboard::Period:
			return hui::KeyCode::Period;
		case sf::Keyboard::Quote:
			return hui::KeyCode::Apostrophe;
		case sf::Keyboard::Slash:
			return hui::KeyCode::Slash;
		case sf::Keyboard::Backslash:
			return hui::KeyCode::Backslash;
		case sf::Keyboard::Tilde:
			return hui::KeyCode::None;
		case sf::Keyboard::Equal:
			return hui::KeyCode::Equals;
		case sf::Keyboard::Hyphen:
			return hui::KeyCode::None;
		case sf::Keyboard::Space:
			return hui::KeyCode::Space;
		case sf::Keyboard::Enter:
			return hui::KeyCode::Enter;
		case sf::Keyboard::Backspace:
			return hui::KeyCode::Backspace;
		case sf::Keyboard::Tab:
			return hui::KeyCode::Tab;
		case sf::Keyboard::PageUp:
			return hui::KeyCode::PgUp;
		case sf::Keyboard::PageDown:
			return hui::KeyCode::PgDown;
		case sf::Keyboard::End:
			return hui::KeyCode::End;
		case sf::Keyboard::Home:
			return hui::KeyCode::Home;
		case sf::Keyboard::Insert:
			return hui::KeyCode::Insert;
		case sf::Keyboard::Delete:
			return hui::KeyCode::Delete;
		case sf::Keyboard::Add:
			return hui::KeyCode::Add;
		case sf::Keyboard::Subtract:
			return hui::KeyCode::Subtract;
		case sf::Keyboard::Multiply:
			return hui::KeyCode::Multiply;
		case sf::Keyboard::Divide:
			return hui::KeyCode::Divide;
		case sf::Keyboard::Left:
			return hui::KeyCode::ArrowLeft;
		case sf::Keyboard::Right:
			return hui::KeyCode::ArrowRight;
		case sf::Keyboard::Up:
			return hui::KeyCode::ArrowUp;
		case sf::Keyboard::Down:
			return hui::KeyCode::ArrowDown;
		case sf::Keyboard::Numpad0:
			return hui::KeyCode::NumPad0;
		case sf::Keyboard::Numpad1:
			return hui::KeyCode::NumPad1;
		case sf::Keyboard::Numpad2:
			return hui::KeyCode::NumPad2;
		case sf::Keyboard::Numpad3:
			return hui::KeyCode::NumPad3;
		case sf::Keyboard::Numpad4:
			return hui::KeyCode::NumPad4;
		case sf::Keyboard::Numpad5:
			return hui::KeyCode::NumPad5;
		case sf::Keyboard::Numpad6:
			return hui::KeyCode::NumPad6;
		case sf::Keyboard::Numpad7:
			return hui::KeyCode::NumPad7;
		case sf::Keyboard::Numpad8:
			return hui::KeyCode::NumPad8;
		case sf::Keyboard::Numpad9:
			return hui::KeyCode::NumPad9;
		case sf::Keyboard::F1:
			return hui::KeyCode::F1;
		case sf::Keyboard::F2:
			return hui::KeyCode::F2;
		case sf::Keyboard::F3:
			return hui::KeyCode::F3;
		case sf::Keyboard::F4:
			return hui::KeyCode::F4;
		case sf::Keyboard::F5:
			return hui::KeyCode::F5;
		case sf::Keyboard::F6:
			return hui::KeyCode::F6;
		case sf::Keyboard::F7:
			return hui::KeyCode::F7;
		case sf::Keyboard::F8:
			return hui::KeyCode::F8;
		case sf::Keyboard::F9:
			return hui::KeyCode::F9;
		case sf::Keyboard::F10:
			return hui::KeyCode::F10;
		case sf::Keyboard::F11:
			return hui::KeyCode::F11;
		case sf::Keyboard::F12:
			return hui::KeyCode::F12;
		case sf::Keyboard::F13:
			return hui::KeyCode::None;
		case sf::Keyboard::F14:
			return hui::KeyCode::None;
		case sf::Keyboard::F15:
			return hui::KeyCode::None;
		case sf::Keyboard::Pause:
			return hui::KeyCode::Pause;
		default:
			break;
		}

		return hui::KeyCode::None;
	};

	auto fromSfmlMouseButton = [](sf::Mouse::Button btn)
	{
		switch (btn)
		{
		case sf::Mouse::Left:
			return hui::MouseButton::Left;
		case sf::Mouse::Right:
			return hui::MouseButton::Right;
		case sf::Mouse::Middle:
			return hui::MouseButton::Middle;
		case sf::Mouse::XButton1:
			return hui::MouseButton::AuxButton1;
		case sf::Mouse::XButton2:
			return hui::MouseButton::AuxButton2;
		default:
			break;
		}

		return hui::MouseButton::None;
	};

	event.window = window;
	
	switch (ev.type)
	{
	case sf::Event::Closed:
		event.type = hui::InputEvent::Type::WindowClose;
		break;
	case sf::Event::Resized:                ///< The window was resized (data in event.size)
		event.type = hui::InputEvent::Type::WindowResize;
		break;
	case sf::Event::LostFocus:              ///< The window lost the focus (no data)
		event.type = hui::InputEvent::Type::WindowLostFocus;
		break;
	case sf::Event::GainedFocus:            ///< The window gained the focus (no data)
		event.type = hui::InputEvent::Type::WindowGotFocus;
		focusedWindow = window;
		break;
	case sf::Event::TextEntered:            ///< A character was entered (data in event.text)
	{
		if (ev.text.unicode < 32)
			break;

		event.type = hui::InputEvent::Type::Text;
		char text[5] = { 0 };
		size_t textLen = 0;
		u32 uni[2] = {0};
		uni[0] = ev.text.unicode;
		hui::unicodeToUtf8(uni, 1, text, 4);
		memcpy(event.text.text, text, strlen(text) + 1);
		break;
	}
	case sf::Event::KeyPressed:             ///< A key was pressed (data in event.key)
	{
		event.type = hui::InputEvent::Type::Key;
		event.key.down = true;
		event.key.code = fromSfmlKey(ev.key.code);
		event.key.modifiers = hui::KeyModifiers::None;
		event.key.modifiers |= ev.key.alt ? hui::KeyModifiers::Alt : hui::KeyModifiers::None;
		event.key.modifiers |= ev.key.control ? hui::KeyModifiers::Control : hui::KeyModifiers::None;
		event.key.modifiers |= ev.key.shift ? hui::KeyModifiers::Shift : hui::KeyModifiers::None;
		break;
	}
	case sf::Event::KeyReleased:            ///< A key was released (data in event.key)
		event.type = hui::InputEvent::Type::Key;
		event.key.down = false;
		event.key.code = fromSfmlKey(ev.key.code);
		event.key.modifiers = hui::KeyModifiers::None;
		event.key.modifiers |= ev.key.alt ? hui::KeyModifiers::Alt : hui::KeyModifiers::None;
		event.key.modifiers |= ev.key.control ? hui::KeyModifiers::Control : hui::KeyModifiers::None;
		event.key.modifiers |= ev.key.shift ? hui::KeyModifiers::Shift : hui::KeyModifiers::None;
		break;
	case sf::Event::MouseWheelScrolled:     ///< The mouse wheel was scrolled (data in event.mouseWheelScroll)
		event.type = hui::InputEvent::Type::MouseWheel;
		event.mouse.wheel.y = ev.mouseWheelScroll.delta;
		event.mouse.point.x = ev.mouseWheelScroll.x;
		event.mouse.point.y = ev.mouseWheelScroll.y;
		break;
	case sf::Event::MouseButtonPressed:     ///< A mouse button was pressed (data in event.mouseButton)
		event.type = hui::InputEvent::Type::MouseDown;
		event.mouse.button = fromSfmlMouseButton(ev.mouseButton.button);
		event.mouse.point.x = ev.mouseButton.x;
		event.mouse.point.y = ev.mouseButton.y;
		event.mouse.clickCount = 1;
		focusedWindow = window;
		break;
	case sf::Event::MouseButtonReleased:    ///< A mouse button was released (data in event.mouseButton)
		event.type = hui::InputEvent::Type::MouseUp;
		event.mouse.button = fromSfmlMouseButton(ev.mouseButton.button);
		event.mouse.point.x = ev.mouseButton.x;
		event.mouse.point.y = ev.mouseButton.y;
		event.mouse.clickCount = 1;
		break;
	case sf::Event::MouseMoved:             ///< The mouse cursor moved (data in event.mouseMove)
        hui::setMouseMoved(true);
        event.type = hui::InputEvent::Type::MouseMove;
        event.mouse.point.x = ev.mouseMove.x;
        event.mouse.point.y = ev.mouseMove.y;
        break;
	case sf::Event::MouseEntered:           ///< The mouse cursor entered the area of the window (no data)
        hoveredWindow = focusedWindow;
		break;
	case sf::Event::MouseLeft:              ///< The mouse cursor left the area of the window (no data)
		break;
	}

	if (ev.type == sf::Event::Closed)
	{
		// give a chance the user to cancel quit, can be cancelled with cancelQuit()
		wantsToQuitApp = true;
	}

    addInputEvent(event);
}

void SfmlInputProvider::startTextInput(hui::Window window, const hui::Rect& imeRect)
{
	
}

void SfmlInputProvider::stopTextInput()
{

}

bool SfmlInputProvider::copyToClipboard(hui::Utf8String text)
{
	sf::Clipboard::setString(text);
	return true;
}

bool SfmlInputProvider::pasteFromClipboard(hui::Utf8String *outText)
{
	auto str = sf::Clipboard::getString().toUtf8();
	
	*outText = new char[str.size()];
	memcpy((char*)(*outText), str.data(), str.size());
	return true;
}

void SfmlInputProvider::processEvents()
{
	for (int i = 1; i < maxWindowCount; i++)
	{
		if (freeWindowSlot[i])
			continue;

		sf::Event ev;

		while (windows[i].pollEvent(ev))
		{
			processSfmlEvent((hui::Window)i, ev);
		}
	}
}

void SfmlInputProvider::setCurrentWindow(hui::Window window)
{
	currentWindow = window;
}

hui::Window SfmlInputProvider::getCurrentWindow()
{
	return currentWindow;
}

hui::Window SfmlInputProvider::getFocusedWindow()
{
	return focusedWindow;
}

hui::Window SfmlInputProvider::getHoveredWindow()
{
	return hoveredWindow;
}

hui::Window SfmlInputProvider::getMainWindow()
{
	return mainWindow;
}

hui::Window SfmlInputProvider::createWindow(
	hui::Utf8String title,
    i32 width, i32 height,
	hui::WindowBorder border,
	hui::WindowPositionType positionType,
	hui::Point customPosition,
	bool showInTaskBar)
{
	int freeIndex = -1;

	for (int i = 1; i < maxWindowCount; i++)
	{
		if (freeWindowSlot[i])
		{
			windows[i].create(sf::VideoMode(width, height), title);
			freeWindowSlot[i] = false;
			windows[i].setPosition({ (int)customPosition.x, (int)customPosition.y });
			return (hui::Window)i;
		}
	}

	return 0;
}

void SfmlInputProvider::setWindowTitle(hui::Window window, hui::Utf8String title)
{
	int index = (int)window;

	windows[index].setTitle(title);
}

void SfmlInputProvider::setWindowRect(hui::Window window, const hui::Rect& rect)
{
	int index = (int)window;

	windows[index].setPosition({ (int)rect.topLeft().x, (int)rect.topLeft().y });
	windows[index].setSize({ (u32)rect.width, (u32)rect.height });
}

hui::Rect SfmlInputProvider::getWindowRect(hui::Window window)
{
	int index = (int)window;

	auto pos = windows[index].getPosition();
	auto size = windows[index].getSize();

	return { (f32)pos.x, (f32)pos.y, (f32)size.x, (f32)size.y };
}

void SfmlInputProvider::presentWindow(hui::Window window)
{
	int index = (int)window;

	windows[index].display();
}

void SfmlInputProvider::destroyWindow(hui::Window window)
{
	int index = (int)window;
	windows[index].close();
	freeWindowSlot[index] = true;
}

void SfmlInputProvider::showWindow(hui::Window window)
{
	int index = (int)window;
	windows[index].setVisible(true);
}

void SfmlInputProvider::hideWindow(hui::Window window)
{
	int index = (int)window;
	windows[index].setVisible(false);
}

void SfmlInputProvider::raiseWindow(hui::Window window)
{
	int index = (int)window;
	windows[index].requestFocus();
}

void SfmlInputProvider::maximizeWindow(hui::Window window)
{
	int index = (int)window;
	//TODO
}

void SfmlInputProvider::minimizeWindow(hui::Window window)
{
	int index = (int)window;
	//TODO
}

hui::WindowState SfmlInputProvider::getWindowState(hui::Window window)
{
	return hui::WindowState::Normal;
}

void SfmlInputProvider::setCapture(hui::Window window)
{
	int index = (int)window;
	windows[index].requestFocus();
}

void SfmlInputProvider::releaseCapture()
{
}

hui::Point SfmlInputProvider::getMousePosition()
{
	auto pos = sf::Mouse::getPosition();
	return { (f32)pos.x, (f32)pos.y };
}

void SfmlInputProvider::setCursor(hui::MouseCursorType type)
{
	//TODO
}

hui::MouseCursor SfmlInputProvider::createCustomCursor(hui::Rgba32* pixels, u32 width, u32 height, u32 hotX, u32 hotY)
{
	for (int i = 0; i < maxCursorCount; i++)
	{
		if (freeCursorSlot[i])
		{
			freeCursorSlot[i] = false;
			cursors[i].loadFromPixels((sf::Uint8*)pixels, { width, height }, { hotX, hotY });
			return (hui::MouseCursor)i;
		}
	}

	return 0;
}

void SfmlInputProvider::destroyCustomCursor(hui::MouseCursor cursor)
{
	int index = (int)cursor;
	freeCursorSlot[index] = true;
}

void SfmlInputProvider::setCustomCursor(hui::MouseCursor cursor)
{
	int index = (int)cursor;

	if (currentWindow)
	{
		windows[(int)currentWindow].setMouseCursor(cursors[(int)cursor]);
	}
}

bool SfmlInputProvider::mustQuit()
{
	return mustQuitApp;
}

bool SfmlInputProvider::wantsToQuit()
{
	return wantsToQuitApp;
}

void SfmlInputProvider::cancelQuitApplication()
{
	mustQuitApp = false;
}

void SfmlInputProvider::quitApplication()
{
	mustQuitApp = true;
}

void SfmlInputProvider::initialize()
{
    for (int i = 0; i < maxWindowCount; i++)
    {
        freeWindowSlot[i] = true;
    }

    for (int i = 0; i < maxCursorCount; i++)
    {
        freeCursorSlot[i] = true;
    }
}

void SfmlInputProvider::shutdown()
{
	//TODO
}

void SfmlInputProvider::updateDeltaTime()
{
	auto time = clock.getElapsedTime();
	u32 ticks = time.asMilliseconds();
	deltaTime = (f32)(ticks - lastTime) / 1000.0f;
	lastTime = ticks;
}

void initializeWithSfml(const SfmlSettings& settings)
{
    Rect wndRect = settings.mainWindowRect;

    if (!settings.gfxProvider)
    {
        printf("SFML needs a graphics provider\n");
        return;
    }

    printf("Initializing SFML...\n");

    SfmlInputProvider* inputProvider = new SfmlInputProvider();

    inputProvider->initialize();

    if (!settings.sfmlContext)
    {
        //inputProvider->sfmlContext = new sf::Context();
    }

    //inputProvider->sfmlContext->setActive(true);
    inputProvider->gfxProvider = settings.gfxProvider;

    bool maximize = false;
    hui::Window wnd = nullptr;

    inputProvider->context = createContext(inputProvider, settings.gfxProvider);

    if (!settings.sfmlMainWindow)
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

        wnd = inputProvider->createWindow(
            settings.mainWindowTitle,
            (int)wndRect.width,
            (int)wndRect.height,
            WindowBorder::Resizable,
            settings.positionType,
            { wndRect.x, wndRect.y },
            true);

        if (!wnd)
        {
            printf("Cannot create SFML window: %s\n", settings.mainWindowTitle);
            return;
        }
    }

    settings.gfxProvider->initialize();
    initializeContext(inputProvider->context);
    inputProvider->mainWindow = wnd;
    inputProvider->focusedWindow = wnd;
}

}
