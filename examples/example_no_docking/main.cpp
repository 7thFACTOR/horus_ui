#pragma execution_character_set("utf-8")
#include "horus.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <filesystem>

// backends
#include "sdl3_input.h"
#include "opengl_graphics.h"
#include "json_theme_loader.h"
#include "stb_rectpack.h"
#include "freetype_fonts.h"
#include "native_file_dialogs.h"
#include "stdio_fileio.h"
#include "utfcpp.h"

int main(int argc, char** args)
{
	// Setup a Horus UI context, with given service providers
	hui::Settings settings;

	//settings.sliderDragDirection = hui::SliderDragDirection::HorizontalOnly;

	// Create the context
	auto huiContext = hui::createContext(settings);
	hui::setContext(huiContext); // set as current context

	// Initialize SDL input provider
	hui::Sdl3InitParams sdlParams;

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


		// Theme file path
		static const char* themeFilePath = "../themes/default.theme.json";

		// Theme reload function (used by F2 key and auto-reload)
		auto reloadTheme = [&]()
			{
				auto newTheme = hui::loadThemeFromJson(themeFilePath, err, errSize);

				if (newTheme)
				{
					// delete old theme
					if (theme) hui::deleteTheme(theme);
					theme = newTheme;
					hui::setTheme(theme);

					// Reload resources
					hui::buildTheme(theme);
					printf("Theme reloaded!\n");
				}
			};

		// Track theme file modification time for auto-reload
		static auto lastModTime = std::filesystem::last_write_time(themeFilePath);
		static f32 checkTimer = 0;

		checkTimer += hui::getFrameDeltaTime();

		// Check if theme file has been modified (every 1 second)
		if (checkTimer >= 1.0f)
		{
			checkTimer = 0;

			try
			{
				auto currentModTime = std::filesystem::last_write_time(themeFilePath);
				if (currentModTime != lastModTime)
				{
					lastModTime = currentModTime;
					reloadTheme();
				}
			}
			catch (...)
			{
				// Ignore filesystem errors
			}
		}

		// Get the events from SDL or whatever input provider is set, it will fill a queue of events
		hui::update();

		if (hui::getInputEvent().type == hui::InputEvent::Type::Key
			&& hui::getInputEvent().key.code == hui::KeyCode::F2
			&& hui::getInputEvent().key.down)
		{
			reloadTheme();
		}

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
				hui::Rect rc = { 30, 30, 700*hui::getScale(), 1500 * hui::getScale() };
				hui::pushPadding(hui::PaddingType::Layout, 0);
				hui::beginLayout(rc);
				hui::pushPadding(hui::PaddingType::Layout, 10);
				
				hui::beginBoxLayout("box1", hui::Color::white, hui::WidgetElementId::WindowBody);

				hui::button("BUTTON0");

				hui::beginBoxLayout("box2", hui::Color::red);
				
				//hui::button("BUTTON1");
				
				hui::beginBoxLayout("box3", hui::Color::blue);
				
				//hui::button("BUTTON2");
				
				hui::beginBoxLayout("box4", hui::Color::green);
				
				hui::button("BUTTON3"); hui::sameLine();
				hui::button("BUTTON3g"); hui::sameLine();
				hui::button("BUTTON3n"); hui::sameLine();
				hui::button("BUTTON3nn"); hui::sameLine();
				hui::button("BUTTONnnnn3");
				hui::button("BUTTONnnnn3");
				hui::button("BUTTONnnnn3");
				
				hui::endBoxLayout();
				hui::endBoxLayout();
				hui::endBoxLayout();

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

				static hui::Point scrollPos = 0;
				hui::space();
				hui::beginScrollView("scrl1", 200, scrollPos, 0, hui::ScrollViewFlags::NoBorder);
				hui::labelMultiline("Lorem ipsum dolor sit amet, consectetur\nvelit esasdf asdf asdf asdf asdfsaf asdf asdf asdf asdf asdf asf asf asdf asdf asdf asfas fasdf asdf asdfasdf asdf asf asf asdf asdf asd fasdf asdf asf asf asdf asf asdf asdf asdf asdf asdf sadf dsf fasf asdf asdf asdf asdfasdf asdf asdfdasdasdfjksadkjf \nESCAPSIMG\n\n\n\naksdf kasjdfk sad fsadjf kjsadf asd fkasdfk sadksakd ksadfsadkf askd fasfsdf asd fasdf asdf asdf skd fsad fasd fasdkfj sadkjf sakdjf askdjf skadf asdf sadf se quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur? Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur? Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur? NU!", hui::HAlignType::Left);
				hui::line();
				hui::button("I AGREE");
				hui::line();

				scrollPos = hui::endScrollView();

				if (hui::beginMenuBar())
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

				hui::labelCustomFont("Once upon a time in the west", hui::getThemeFont(theme, "title"), hui::HAlignType::Center);
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

				//hui::sameLine();
				hui::check("A simple check box", &chk);
				hui::button("Check me1!");
				hui::sameLine();
				hui::button("Check me2! Other text");
				hui::sameLine();
				hui::button("HOKA");

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

				if (hui::button("SHOW POPUP"))
				{
					popup = true;
				}

				if (popup)
				{
					hui::beginPopup("_popup", 300, hui::PopupFlags::Centered|hui::PopupFlags::FadeBackground);
					hui::label("A sample popup"); hui::sameLine();
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

				static hui::Point scrollPos2 = 0;
				hui::pushPadding(hui::PaddingType::ScrollView, { 10, 0 });
				hui::beginScrollView("scrl2", 180, scrollPos2.x);
				hui::popPadding(hui::PaddingType::ScrollView);
				hui::rotarySliderFloat("Speed", &val, -30, 100, 1, false);
				static char txt[1000];
				hui::textInput("txt1", txt, 1000);
				hui::comboSliderFloat(&val, 1, 1, "%.4f °");
				hui::comboSliderFloatRanged(&val2, 0, 100, 1, 1, "%.4f cm");
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
					hui::pushId(i);
					hui::button("Accelerate1"); hui::sameLine();
					hui::button("Accelerate2"); hui::sameLine();
					hui::button("Accelerate3"); hui::sameLine();
					hui::button("Accelerate4"); hui::sameLine();
					hui::button("Accelerate5"); hui::sameLine();
					hui::button("Accelerate6"); hui::sameLine();
					hui::button("Accelerate7"); hui::sameLine();
					hui::button("Accelerate8");
					hui::popId();
				}

				
				hui::pushTint(hui::Color::red);
				hui::button("  EXIT    ");
				hui::popTint();
				hui::pushTint(hui::Color(1,0,0,1), hui::TintColorType::Body, hui::TintColorOpType::Replace);
				hui::sameLine();
				hui::button("   ABORT   ");
				hui::popTint();
				hui::pushTint(hui::Color::sky, hui::TintColorType::Text);
				hui::sameLine();
				hui::button("QUIT APPLICATION");
				hui::popTint();
				

				hui::space(20);
				
				hui::popWidgetPadding();
				hui::endBoxLayout();
				hui::endLayout();
				hui::popPadding(hui::PaddingType::Layout);
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