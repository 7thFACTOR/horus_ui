#pragma execution_character_set("utf-8")
#include "horus.h"

#define _USE_MATH_DEFINES
#include <cmath>

// backends
#include "sdl3_input_provider.h"
#include "opengl_graphics_provider.h"
#include "opengl_vertex_buffer.h"
#include "opengl_texture_array.h"
#include "stb_image_provider.h"
#include "json_theme_provider.h"
#include "stb_rectpack_provider.h"
#include "freetype_font_provider.h"
#include "nativefiledialogs_provider.h"
#include "stdio_file_provider.h"
#include "utfcpp_provider.h"

hui::WidgetElementInfo inf;

void curveEditor(f32 height, u32 maxPoints, hui::Point* points, u32& pointCount, const hui::Color& lineColor)
{
	hui::beginCustomWidget("curveEditor", height);
	hui::setColor(hui::isPressed() ? hui::Color::red : hui::Color::white);
	hui::drawBorderedImage(inf.image, inf.border, hui::getWidgetRect());
	hui::setLineStyle({ hui::Color::orange, 1.5f });
	hui::drawLine({ 10,10 }, { 10,110 });
	hui::SplineControlPoint p[4];
	p[0].center = { 10, 20 };
	p[0].rightTangent = { 10,20 };
	p[1].center = { 210, 120 };
	p[1].leftTangent = { -10,520 };
	p[1].rightTangent = { 10,20 };

	p[2].center = { 310, 320 };
	p[2].isLine = false;
	p[2].leftTangent = { -10,20 };
	p[2].rightTangent = { 10,20 };

	p[3].center = hui::getMousePosition();
	p[3].isLine = false;
	p[3].leftTangent = { -10,20 };
	p[3].rightTangent = { 10,20 };
	hui::drawSpline(p, 4, 100);
	hui::endCustomWidget();
}

int main(int argc, char** args)
{
	// Setup a Horus UI context, with given service providers
	hui::Settings settings;

	settings.providers.file = new hui::StdioFileProvider();
	settings.providers.fileDialogs = new hui::NativeFileDialogsProvider();
	settings.providers.font = new hui::FreetypeFontProvider();
	settings.providers.gfx = new hui::OpenGLGraphicsProvider();
	settings.providers.image = new hui::StbImageProvider();
	settings.providers.input = new hui::Sdl3InputProvider();
	settings.providers.rectPack = new hui::StbRectPackProvider();
	settings.providers.utf = new hui::UtfCppProvider();

	// Create the context
	auto huiContext = hui::createContext(settings);
	hui::setContext(huiContext); // set as current context

	// Initialize SDL input provider
	hui::SdlInitParams sdlParams;

	sdlParams.vSync = false;
	hui::initializeSdl(sdlParams);

	// Create the main window (this will also create a graphics (GL/VK/D3D/etc.) context)
	auto mainWnd = HORUS_INPUT->createWindow("Horus Example - Custom Widget", hui::NativeWindowFlags::Resizable, hui::NativeWindowState::Maximized, hui::Rect(0, 0, 1000, 800));

	// Initialize the graphics API, since now we have a first window created
	// (we cant initialize the graphics api without a window)
	HORUS_GFX->initialize();

	// Initialize the UI renderer for the current context
	hui::initializeRenderer();

	// Load a theme
	const u32 errSize = 2048;
	char err[errSize] = { 0 };
	auto theme = hui::loadThemeFromJson("../themes/default.theme.json", err, errSize);

	if (!theme)
	{
		printf("Theme JSON error: %s\n", err);
		exit(1);
	}

	// Set the current theme
	hui::setTheme(theme);

	// Build the theme
	// After we load the theme and more images and fonts, we need to rebuild the theme (into the image atlas)
	hui::buildTheme(theme);

	hui::getThemeWidgetElementInfo(hui::WidgetElementId::BoxBody, hui::WidgetStateType::Normal, inf);

	// Start the main loop
	bool exitNow = false;

	while (!exitNow)
	{
		// Clear the main window as a test
		HORUS_INPUT->setCurrentWindow(mainWnd);
		glClearColor(0.4f, 0.4f, 0.4f, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		// Get the events from SDL or whatever input provider is set, it will fill a queue of events
		hui::update();

		// Check the event count
		auto eventCount = hui::getInputEventCount();

		// the main frame rendering and input handling
		auto doFrame = [&](bool lastEventInQueue)
		{
			hui::setCurrentNativeWindow(mainWnd);
			hui::beginRendering();
			// Begin an actual frame of the gui
			hui::beginFrame();
			// disable rendering if its not the last event in the queue
			// no need to render while handling all the input events
			// we only render on the last event in the queue
			hui::setDisableRendering(!lastEventInQueue);

			const int maxPts = 32;
			hui::Point pts[maxPts] = { 0 };
			u32 ptCount = 0;

			hui::Rect rc = {30, 30, 500, 400};

			hui::beginContainer(rc);
			curveEditor(55, maxPts, pts, ptCount, hui::Color::red);
			hui::endContainer();

			hui::endFrame();
			hui::endRendering();

			if (lastEventInQueue)
				hui::presentNativeWindow(mainWnd);
		};

		// if we have events, then go through all of them and call the frame render and input
		if (eventCount)
		{
			for (auto i = 0; i < eventCount; i++)
			{
				hui::setInputEvent(hui::getInputEventAt(i));

				if (hui::getInputEvent().type == hui::InputEvent::Type::WindowClose)
				{
					if (hui::getInputEvent().window == mainWnd)
					{
						exitNow = true;
					}
				}

				doFrame(i == eventCount - 1);
			}
		}
		else
		{
			// if no events, just draw the frame
			doFrame(true);
		}
	}

	hui::shutdown();
	hui::deleteContext(huiContext);

	// delete owned pointers
	delete settings.providers.file;
	delete settings.providers.fileDialogs;
	delete settings.providers.font;
	delete settings.providers.gfx;
	delete settings.providers.image;
	delete settings.providers.input;
	delete settings.providers.rectPack;
	delete settings.providers.utf;

	return 0;
}