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
	//settings.sliderDragDirection = hui::SliderDragDirection::HorizontalOnly;

	// Create the context
	auto huiContext = hui::createContext(settings);
	hui::setContext(huiContext); // set as current context

	// Initialize SDL input provider
	hui::SdlInitParams sdlParams;

	sdlParams.vSync = false;
	hui::initializeSdl(sdlParams);

	// Create the main window (this will also create a graphics (GL/VK/D3D/etc.) context)
	auto mainWnd = HORUS_INPUT->createWindow("Horus Example - No Docking", hui::NativeWindowFlags::Resizable, hui::NativeWindowState::Maximized, hui::Rect(0, 0, 1000, 800));

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

	// Start the main loop
	bool exitNow = false;

	hui::changeScale(1.0f);

	while (!exitNow)
	{
		// Clear the main window as a test
		HORUS_INPUT->setCurrentWindow(mainWnd);
		glClearColor(0.1f, 0.4f, 0.4f, 1);
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
				hui::Rect rc = { 30, 30, 1200*hui::getScale(), 1500 * hui::getScale() };
				hui::beginLayout(rc);
				hui::pushPadding({10, 10});
				hui::beginBox(hui::Color::white, hui::WidgetElementId::WindowBody, hui::WidgetStateType::Normal);
				hui::popPadding();
				hui::customSpace(20);

				hui::labelCustomFont("Once upon a time...", hui::getThemeFont(theme, "title"), hui::HAlignType::Center);
				hui::line();
				
				if (hui::button("Do not push this button"))
				{
					printf("No you haven't!");
				}

				static bool chk = true;
				hui::check("A simple check box", &chk);
				
				static i32 option1 = 0;
				static i32 option2 = 0;
				static bool showRadios = true;
				hui::pushTint(hui::Color::orange);
				if (hui::panel("Radios 1##1"))
				{
					hui::radio("Radio value 0", &option1, 0);
					hui::radio("Radio value 1", &option1, 1);
					hui::radio("Radio value 2", &option1, 2);
				}
				hui::popTint();

				static f32 val;
				static f32 val2;

				static f32 scrollPos = 0;
				hui::pushPadding({ 10, 0 });
				hui::beginScrollView(180, scrollPos);
				hui::popPadding();
				hui::rotarySliderFloat("Speed", &val, -30, 100, 1, false);
				static char txt[1000];
				hui::textInput(txt, 1000);

				hui::comboSliderFloat(&val, 1, 1, "°");

				hui::comboSliderFloat(&val2, 1, 1, "cm");
				static i32 sel = 0;

				const char* items[6] = { "aaa", "bbbb", "ccccc", "ddd", "eeee", "ffff"};

				hui::dropdown("m", sel, items, 6, 3);

				if (hui::panel("Radios 2##2"))
				{
					hui::radio("Radio value 0", &option2, 0);
					hui::radio("Radio value 1", &option2, 1);
					hui::radio("Radio value 2", &option2, 2);
				}

				hui::line();
				
				scrollPos = hui::endScrollView();

				if (hui::panel("Many buttons##3"))
				for (int i = 0; i < 20; i++)
				{
					hui::beginSameLine();
					hui::button("Accelerate");
					hui::button("Accelerate");
					hui::button("Accelerate");
					hui::button("Accelerate");
					hui::button("Accelerate");
					hui::button("Accelerate");
					hui::button("Accelerate");
					hui::button("Accelerate");
					hui::button("Accelerate");
					hui::button("Accelerate");
					hui::button("Accelerate");
					hui::button("Accelerate");
					hui::button("Accelerate");
					hui::endSameLine();
				}

				hui::beginSameLine(5);
				hui::pushTint(hui::Color::red);
				hui::button("  EXIT  ");
				hui::popTint();
				hui::pushTint(hui::Color(1,0,0,1), hui::TintColorType::Body, hui::TintColorOpType::Replace);
				hui::button("  ABORT  ");
				hui::popTint();
				hui::pushTint(hui::Color::sky, hui::TintColorType::Text);
				hui::button("  QUIT APPLICATION ");
				hui::popTint();
				hui::endSameLine();

				hui::customSpace(20);
				
				hui::endBox();
				hui::endLayout();
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