#include <horus.h>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <GL/gl.h>
#pragma execution_character_set("utf-8")
#include <vector>
#include <string>
#define SFML_STATIC
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <map>
#include <horus_interfaces.h>
#include <string.h>

struct SfmlInputProvider : hui::InputProvider
{
    static const int maxWindowCount = 500;
    static const int maxCursorCount = 500;
    sf::Window windows[maxWindowCount];
    bool freeWindowSlot[maxWindowCount] = { true };
    sf::Cursor cursors[maxCursorCount];
    bool freeCursorSlot[maxCursorCount] = { true };
    sf::Window* currentWindow = nullptr;
    sf::Window* focusedWindow = nullptr;
    sf::Window* hoveredWindow = nullptr;
    sf::Window* mainWindow = nullptr;
    sf::Clock clock;
    std::vector<hui::InputEvent> events;
    bool wantsToQuitApp = false;
    bool mustQuitApp = false;
    bool disableMouseMove = false;
    f32 deltaTime = 0;
    int lastTime = 0;

    void processSfmlEvent(sf::Window* window, sf::Event& ev)
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
			char text[hui::InputEvent::TextData::maxTextBufferSize] = { 0 };
			size_t textLen = 0;
            u32 uni[2] = {0};
            uni[0] = ev.text.unicode;
			hui::unicodeToUtf8(uni, 1, text, hui::InputEvent::TextData::maxTextBufferSize);
			memcpy(event.text.text, text, strlen(text));
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
			event.type = hui::InputEvent::Type::MouseMove;
			event.mouse.point.x = ev.mouseMove.x;
			event.mouse.point.y = ev.mouseMove.y;
			break;
        case sf::Event::MouseEntered:           ///< The mouse cursor entered the area of the window (no data)
			break;
        case sf::Event::MouseLeft:              ///< The mouse cursor left the area of the window (no data)
            break;
        }

		if (ev.type == sf::Event::Closed)
		{
			// give a chance the user to cancel quit, can be cancelled with cancelQuit()
			wantsToQuitApp = true;
		}

		events.push_back(event);
    }

    bool popEvent(hui::InputEvent* outEvent) override
    {
		bool hasEvents = events.size() != 0;

		if (!hasEvents)
			return false;

		*outEvent = events.front();
		events.erase(events.begin());

        return hasEvents;
    }

    void startTextInput(hui::Window window, const hui::Rect& imeRect) override
    {
        
    }

    void stopTextInput() override
    {

    }

    bool copyToClipboard(hui::Utf8String text) override
    {
		sf::Clipboard::setString(text);
		return true;
    }

    bool pasteFromClipboard(hui::Utf8String *outText) override
    {
		//auto str = sf::Clipboard::getString().toUtf8();
		
		//*outText = new char[str.size()];
		//memcpy((char*)(*outText), str.data(), str.size());
		return true;
	}

    u32 getEventCount() const override
    {
		return events.size();
    }

    void processEvents() override
    {
		for (int i = 0; i < maxWindowCount; i++)
		{
            if (freeWindowSlot[i])
                continue;

			sf::Event ev;

			while (windows[i].pollEvent(ev))
			{
				processSfmlEvent(&windows[i], ev);
			}
		}
    }

    void flushEvents() override
    {
		events.clear();
    }

    void setCurrentWindow(hui::Window window) override
    {
		currentWindow = (sf::Window*)window;
    }

    hui::Window getCurrentWindow() override
    {
		return currentWindow;
    }

    hui::Window getFocusedWindow() override
    {
        return focusedWindow;
    }

    hui::Window getHoveredWindow() override
    {
        return hoveredWindow;
    }

    hui::Window getMainWindow() override
    {
        return mainWindow;
    }

    hui::Window createWindow(
        hui::Utf8String title, i32 width, i32 height,
        hui::WindowBorder border = hui::WindowBorder::Resizable,
        hui::WindowPositionType positionType = hui::WindowPositionType::Undefined,
        hui::Point customPosition = { 0, 0 },
        bool showInTaskBar = true) override
    {
        int freeIndex = -1;

        for (int i = 0; i < maxWindowCount; i++)
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

    void setWindowTitle(hui::Window window, hui::Utf8String title) override
    {
        int index = (int)window;

        windows[index].setTitle(title);
    }

    void setWindowRect(hui::Window window, const hui::Rect& rect) override
    {
        int index = (int)window;

        windows[index].setPosition({ (int)rect.topLeft().x, (int)rect.topLeft().y });
        windows[index].setSize({ (u32)rect.width, (u32)rect.height });
    }

	hui::Rect getWindowRect(hui::Window window) override
    {
        int index = (int)window;

        auto pos = windows[index].getPosition();
        auto size = windows[index].getSize();

        return { (f32)pos.x, (f32)pos.y, (f32)size.x, (f32)size.y };
    }

    void presentWindow(hui::Window window) override
    {
        int index = (int)window;

        windows[index].display();
    }

    void destroyWindow(hui::Window window) override
    {
        int index = (int)window;
        windows[index].close();
        freeWindowSlot[index] = true;
    }

    void showWindow(hui::Window window) override
    {
        int index = (int)window;
        windows[index].setVisible(true);
    }

    void hideWindow(hui::Window window) override
    {
        int index = (int)window;
        windows[index].setVisible(false);
    }

    void raiseWindow(hui::Window window) override
    {
        int index = (int)window;
        windows[index].requestFocus();
    }

    void maximizeWindow(hui::Window window) override
    {
        int index = (int)window;
        //TODO
    }

    void minimizeWindow(hui::Window window) override
    {
        int index = (int)window;
        //TODO
    }

	hui::WindowState getWindowState(hui::Window window) override
    {
        return hui::WindowState::Normal;
    }

    void setCapture(hui::Window window) override
    {
        int index = (int)window;
        windows[index].requestFocus();
    }

    void releaseCapture() override
    {
    }

	hui::Point getMousePosition() override
    {
        auto pos = sf::Mouse::getPosition();
        return { (f32)pos.x, (f32)pos.y };
    }

    void setCursor(hui::MouseCursorType type) override
    {
        //TODO
    }

	hui::MouseCursor createCustomCursor(hui::Rgba32* pixels, u32 width, u32 height, u32 hotX, u32 hotY) override
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

    void destroyCustomCursor(hui::MouseCursor cursor) override
    {
        int index = (int)cursor;
        freeCursorSlot[index] = true;
    }

    void setCustomCursor(hui::MouseCursor cursor) override
    {
        int index = (int)cursor;

        if (currentWindow)
        {
            currentWindow->setMouseCursor(cursors[(int)cursor]);
        }
    }

    bool mustQuit() override
    {
        return mustQuitApp;
    }

    bool wantsToQuit() override
    {
        return wantsToQuitApp;
    }

    void cancelQuitApplication() override
    {
        mustQuitApp = false;
    }

    void quitApplication() override
    {
        mustQuitApp = true;
    }

    void shutdown() override
    {
        //TODO
    }

    void updateDeltaTime() override
    {
        auto time = clock.getElapsedTime();
        u32 ticks = time.asMilliseconds();
        deltaTime = (f32)(ticks - lastTime) / 1000.0f;
        lastTime = ticks;
    }

    f32 getDeltaTime() const override
    {
        return deltaTime;
    }

    void disableMouseMoveEvents(bool disable) override
    {
        disableMouseMove = disable;
    }
};

int main(int argc, char** args)
{
    auto huiCtx = hui::createContext(hui::GraphicsApi::OpenGL, new SfmlInputProvider);
    auto wnd = hui::createWindow("Sfml", 1000, 800);
    hui::setWindow(wnd);
	auto theme = hui::loadTheme("../data/default.theme");
	hui::setTheme(theme);

    auto largeFnt = hui::getFont(theme, "title");

    while (true)
    {
        hui::processEvents();
        hui::setWindow(hui::getMainWindow());
        hui::beginWindow(hui::getMainWindow());

        // user drawing code
        glClearColor(0, .3, .2, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        static f32 x = 1;
        static f32 t = 1;
        glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0);
        glVertex2f(0, 0);
        glColor3f(1, 1, 0);
        glVertex2f(x, 0);
        glColor3f(1, 0, 1);
        glVertex2f(x, 1);
        glEnd();
        x = sinf(t);
        t += 0.01f;

        // horus ui
        hui::beginFrame();
        hui::Rect panelRect = { 50, 50, 350, 500 };
        hui::beginContainer(panelRect);
        hui::WidgetElementInfo elemInfo;
        hui::getThemeWidgetElementInfo(hui::WidgetElementId::PopupBody, hui::WidgetStateType::Normal, elemInfo);
        hui::setBackColor(hui::Color::white);
        hui::drawBorderedImage(elemInfo.image, elemInfo.border, panelRect);
        hui::pushPadding(15);
        hui::gap(15);
        hui::labelCustomFont("Information", largeFnt);
        hui::button("Activate shields");
        static bool chk1, chk2, chk3;
        hui::beginTwoColumns();
        chk1 = hui::check("Option 1", chk1);
        chk2 = hui::check("Option 2", chk2);
        hui::nextColumn();
        chk3 = hui::check("Option 3", chk3);
        hui::pushTint(hui::Color::cyan);
        hui::button("Browse...");
        hui::popTint();
        hui::endColumns();
        static float val;
        hui::sliderFloat(0, 100, val);
        static char txt[2000];
        hui::textInput(txt, 2000, hui::TextInputValueMode::Any, "Write something here");
        hui::space();

        static f32 scrollPos = 0;
        hui::beginScrollView(200, scrollPos);
        hui::multilineLabel("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur?", hui::HAlignType::Left);
        hui::line();
        hui::button("I AGREE");
        scrollPos = hui::endScrollView();

        if (hui::button("Exit"))
            hui::quitApplication();

        hui::popPadding();
        hui::endContainer();
        hui::endFrame();
        hui::endWindow();
        hui::presentWindow(hui::getMainWindow());

        if (hui::wantsToQuit() || hui::mustQuit())
            break;
    }

    hui::shutdown();


	return 0;
}
