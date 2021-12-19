#include "horus.h"
#include <horus_interfaces.h>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>

namespace hui
{
struct SfmlSettings
{
    char* mainWindowTitle;
    Rect mainWindowRect;
    WindowPositionType positionType = WindowPositionType::Undefined;
    bool vSync = true;
    void* sfmlContext = nullptr;
    sf::Window* sfmlMainWindow = nullptr;
    GraphicsProvider* gfxProvider = nullptr;
};

struct SfmlInputProvider : hui::InputProvider
{
    static const int maxWindowCount = 500;
    static const int maxCursorCount = 500;
    sf::Window windows[maxWindowCount];
    bool freeWindowSlot[maxWindowCount];
    sf::Cursor cursors[maxCursorCount];
    bool freeCursorSlot[maxCursorCount];
    hui::Window currentWindow = 0;
    hui::Window focusedWindow = 0;
    hui::Window hoveredWindow = 0;
    hui::Window mainWindow = 0;
    sf::Clock clock;
    sf::Context* sfmlContext = nullptr;
    GraphicsProvider* gfxProvider = nullptr;
    hui::Context context;
    bool wantsToQuitApp = false;
    bool mustQuitApp = false;
    bool disableMouseMove = false;
    f32 deltaTime = 0;
    int lastTime = 0;

    void processSfmlEvent(hui::Window window, sf::Event& ev);
    void startTextInput(hui::Window window, const hui::Rect& imeRect) override;
    void stopTextInput() override;
    bool copyToClipboard(const char* text) override;
    bool pasteFromClipboard(char* outText, u32 maxTextSize) override;
    void processEvents() override;
    void setCurrentWindow(hui::Window window) override;
    hui::Window getCurrentWindow() override;
    hui::Window getFocusedWindow() override;
    hui::Window getHoveredWindow() override;
    hui::Window getMainWindow() override;
    hui::Window createWindow(
        const char* title, i32 width, i32 height,
        hui::WindowBorder border = hui::WindowBorder::Resizable,
        hui::WindowPositionType positionType = hui::WindowPositionType::Undefined,
        hui::Point customPosition = { 0, 0 },
        bool showInTaskBar = true) override;
    void setWindowTitle(hui::Window window, const char* title) override;
    void setWindowRect(hui::Window window, const hui::Rect& rect) override;
    hui::Rect getWindowRect(hui::Window window) override;
    void presentWindow(hui::Window window) override;
    void destroyWindow(hui::Window window) override;
    void showWindow(hui::Window window) override;
    void hideWindow(hui::Window window) override;
    void raiseWindow(hui::Window window) override;
    void maximizeWindow(hui::Window window) override;
    void minimizeWindow(hui::Window window) override;
    hui::WindowState getWindowState(hui::Window window) override;
    void setCapture(hui::Window window) override;
    void releaseCapture() override;
    hui::Point getMousePosition() override;
    void setCursor(hui::MouseCursorType type) override;
    hui::MouseCursor createCustomCursor(hui::Rgba32* pixels, u32 width, u32 height, u32 hotX, u32 hotY) override;
    void deleteCustomCursor(hui::MouseCursor cursor) override;
    void setCustomCursor(hui::MouseCursor cursor) override;
    bool mustQuit() override;
    bool wantsToQuit() override;
    void cancelQuitApplication() override;
    void quitApplication() override;
    void initialize();
    void shutdown() override;
    void updateDeltaTime();
};

void initializeWithSfml(const SfmlSettings& settings);

}
