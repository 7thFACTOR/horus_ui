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

	hui::changeScale(2.0f);

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
				hui::pushWidgetPadding(10);
				hui::beginBox(hui::Color::white, hui::WidgetElementId::WindowBody, hui::WidgetStateType::Normal);
				hui::popWidgetPadding();
				hui::pushWidgetPadding(0);
				hui::space(20);
				static hui::TabIndex selTab = 0;

				hui::beginTabGroup(selTab);
				hui::tab("One", 0);
				hui::tab("Two", 0);
				hui::tab("Three", 0);
				hui::tab("Four", 0);
				selTab = hui::endTabGroup();

				static f32 scrollPos = 0;
				hui::space();
				hui::beginScrollView(200, scrollPos);
				hui::labelMultiline("Lorem ipsum dolor sit amet, consectetur\nvelit esasdf asdf asdf asdf asdfsaf asdf asdf asdf asdf asdf asf asf asdf asdf asdf asfas fasdf asdf asdfasdf asdf asf asf asdf asdf asd fasdf asdf asf asf asdf asf asdf asdf asdf asdf asdf sadf dsf fasf asdf asdf asdf asdfasdf asdf asdfdasdasdfjksadkjf \nESCAPSIMG\n\n\n\naksdf kasjdfk sad fsadjf kjsadf asd fkasdfk sadksakd ksadfsadkf askd fasfsdf asd fasdf asdf asdf skd fsad fasd fasdkfj sadkjf sakdjf askdjf skadf asdf sadf se quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur? Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur? Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur? NU!", hui::HAlignType::Left);
				hui::line();
				hui::button("I AGREE");
				hui::line();

				scrollPos = hui::endScrollView();

				if (0&&hui::beginMenuBar())
				{
					if (hui::beginMenu("File##1"))
					{
						hui::menuItem("New", "Ctrl+N");
						hui::menuItem("Open", "Ctrl+O");
						hui::menuItem("Print", "Ctrl+P");
						hui::menuSeparator();
						hui::menuItem("Exit", "Alt+F4");
						hui::endMenu();
					}

					if (hui::beginMenu("Edit##2"))
					{
						hui::menuItem("Cut", "Ctrl+X");
						hui::menuItem("Copy", "Ctrl+C");
						hui::menuItem("Paste", "Ctrl+V");
						hui::menuItem("Delete", "Del");
						hui::endMenu();
					}

					if (hui::beginMenu("View"))
					{
						hui::menuItem("Close", 0);
						hui::menuItem("Close All", 0);
						hui::endMenu();
					}

					hui::endMenuBar();
				}

				hui::labelCustomFont("Once upon a time...", hui::getThemeFont(theme, "title"), hui::HAlignType::Center);
				hui::line();
				
				if (hui::button("Do not push this button"))
				{
					printf("No you haven't!");
				}
				hui::tooltip("Button to push");

				static f32 pv = 0, pv2 = 0;
				hui::progress(pv);
				hui::tooltip("The real progress");
				hui::progress(pv2, 2500, true, true);
				hui::tooltip("The other progress");
				hui::progress(pv2, 1700, true, false);
				hui::progress(-1, 0, true, false, "Searching records...");

				if (hui::beginContextMenu())
				{
					hui::menuItem("Delete");
					hui::menuItem("Copy");
					hui::menuItem("Restore");

					hui::endContextMenu();
				}

				pv += 0.01f;
				if (pv > 1) pv = 1;
				pv2 += 0.5f;

				static bool chk = true;
				hui::check("A simple check box", &chk);

				if (hui::beginCustomTooltip(160))
				{
					hui::pushTint(hui::Color::black, hui::TintColorType::Text);
					hui::labelCustomFont("Header", hui::getFont("medium-bold"));
					hui::pushWidgetPadding(0);
					hui::label("Brief explanation");
					hui::line();
					hui::labelMultiline("A longer explanation\nthat needs to explain what is to be explained because of corse its needed.", hui::HAlignType::Left);
					hui::popWidgetPadding();
					hui::popTint();
					hui::endCustomTooltip();
				}

				static bool popup = false;

				if (hui::button("POPUP"))
				{
					popup = true;
				}

				if (popup)
				{
					hui::beginPopup("_popup", 300, hui::PopupFlags::Centered|hui::PopupFlags::FadeBackground);
					hui::label("A sample popup");
					if (hui::button("Close this"))
					{
						hui::closePopup();
						popup = false;
					}
					hui::endPopup();
				}

				static f32 sli = 0;
				hui::sliderFloat("x", 0, 1, sli, false);
				static i32 option1 = 0;
				static i32 option2 = 0;
				static bool showRadios = true;
				hui::pushTint(hui::Color::orange);
				if (hui::expandable("Radios 1##1"))
				{
					hui::radio("Radio value 0", &option1, 0);
					hui::radio("Radio value 1", &option1, 1);
					hui::radio("Radio value 2", &option1, 2);
				}
				hui::popTint();

				static f32 val;
				static f32 val2;

				static f32 scrollPos2 = 0;
				hui::pushPadding(hui::PaddingType::ScrollView, { 10, 0 });
				hui::beginScrollView(180, scrollPos2);
				hui::popPadding(hui::PaddingType::ScrollView);
				hui::rotarySliderFloat("Speed", &val, -30, 100, 1, false);
				static char txt[1000];
				hui::textInput(txt, 1000);
				hui::comboSliderFloat(&val, 1, 1, "°");
				hui::comboSliderFloatRanged(&val2, 0, 100, 1, 1, "cm");
				static i32 sel = 0;

				const char* items[6] = { "One", "Two", "Three", "Four", "Five Hundred Billion Trillion", "Six"};
				//hui::pushWidth(120);
				hui::dropdown("m", sel, items, 6, 3);
				hui::label("TEST");
				//hui::popWidth();

				if (hui::expandable("Radios 2##2"))
				{
					hui::radio("Radio value 0", &option2, 0);
					hui::radio("Radio value 1", &option2, 1);
					hui::radio("Radio value 2", &option2, 2);
				}

				hui::line();
				
				scrollPos2 = hui::endScrollView();

				if (hui::expandable("Many buttons##3"))
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

				hui::space(20);
				
				hui::popWidgetPadding();
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