#pragma execution_character_set("utf-8")
#include "horus.h"
#include <string.h>
#include <string>
#include <unordered_map>

// backends
#include "sdl2_input_provider.h"
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

using namespace hui;
HView view1;
HView view2;
HView view3;
HImage tabIcon1;
HImage tabIcon2;
HImage tabIcon3;

struct View1Data
{
	int data = 1;
};

struct View2Data
{
	int data = 2;
};

struct View3Data
{
	int data = 3;
};

void createMyDefaultViewPanes()
{
	auto myRoot = hui::createRootDockNode(hui::getMainWindow());
	auto view1 = hui::createView(myRoot, hui::DockType::Left, "Assets", 0, 1, 0, nullptr);
	hui::debugViews();
	auto view2 = hui::createView(myRoot, hui::DockType::Left, "Game", 0, 2, 0, nullptr);
	hui::debugViews();
	auto view3 = hui::createView(myRoot, hui::DockType::Top, "Particles", 0, 3, 0, nullptr);
	hui::debugViews();
	hui::dockView(view1, hui::getViewDockNode(view3), hui::DockType::Top);
	hui::debugViews();
	hui::dockView(view2, hui::getViewDockNode(view3), hui::DockType::Right);
	hui::debugViews();
	//hui::dockView(view1, hui::getViewDockNode(view2), hui::DockType::Right);
	hui::debugViews();
	auto view4 = hui::createView(myRoot, hui::DockType::Bottom, "Inspector", 0, 4, 0, nullptr);
	hui::debugViews();
	hui::dockView(view1, hui::getViewDockNode(view4), hui::DockType::Bottom);
	hui::debugViews();
}

int main(int argc, char** args)
{
	///
	/// Setup a Horus UI context, with given service providers
	///
	hui::ContextSettings settings;

	settings.providers.file = new hui::StdioFileProvider();
	settings.providers.fileDialogs = new hui::NativeFileDialogsProvider();
	settings.providers.font = new hui::FreetypeFontProvider();
	settings.providers.gfx = new hui::OpenGLGraphicsProvider();
	settings.providers.image = new hui::StbImageProvider();
	settings.providers.input = new hui::Sdl2InputProvider();
	settings.providers.rectPack = new hui::StbRectPackProvider();
	settings.providers.utf = new hui::UtfCppProvider();

	auto ctx = hui::createContext(settings);
	hui::setContext(ctx); // set as current context

	///
	/// Setup SDL2
	///
	hui::SdlSettings sdlSettings;

	sdlSettings.mainWindowTitle = "HorusUI Example - Widget Showroom";
	sdlSettings.mainWindowRect = { 0, 0, 1200, 1000 };
	hui::setupSDL(sdlSettings); // setup SDL (will create an SDL context if none set in the sdlSettings
	HORUS_GFX->initialize(); // init the gfx objects, since we have now a graphics context set
	hui::initializeRenderer(); // init UI renderer for the current context

	//////////////////////////////////////////////////////////////////////////
	/// Now setup our UI, load theme, etc.

	printf("Loading theme...\n");
	//TODO: load themes and images and anything from memory also
	auto theme = hui::loadThemeFromJson("../themes/default.theme.json");
	hui::setTheme(theme); // set as the current theme

	printf("Loading images...\n");

	tabIcon1 = hui::loadImage("../themes/default/tabicon1.png");
	tabIcon2 = hui::loadImage("../themes/default/tabicon2.png");
	tabIcon2 = hui::loadImage("../themes/default/tabicon3.png");

	// after we load and create images and fonts, rebuild the theme atlas
	hui::buildTheme(theme);

	printf("Loading views...\n");

	// if there is no state, then create the default panes and tabs
	if (!hui::loadDockingState("layout.hui"))
	{
		createMyDefaultViewPanes();
	}

	printf("Starting loop...\n");

	bool exitNow = false;

	do
	{
		// get the events from SDL or whatever input provider is set, it will fill a queue of events
		hui::processInputEvents();
		// lets check the event count
		int eventCount = hui::getInputEventCount();

		// the main frame rendering and input handling
		auto doFrame = [&](bool lastEventInQueue)
		{
			// disable rendering if its not the last event in the queue
			// no need to render while handling all the input events
			// we only render on the last event in the queue
			hui::setDisableRendering(!lastEventInQueue);
			// begin an actual frame of the gui
			hui::beginFrame();

			//if (hui::beginView(view1))
			//{

			//}
			//hui::endView();

			//if (hui::beginView(view2))
			//{

			//}
			//hui::endView();

			//if (hui::beginView(view3))
			//{

			//}
			//hui::endView();

			hui::endFrame();

			if (lastEventInQueue)
			{
				//hui::presentWindows();
			}

			if (hui::wantsToQuit() || hui::mustQuit())
			{
				exitNow = true;
			}
		};

		// if we have events, then go through all of them and call the frame render and input
		if (eventCount)
		{
			for (int i = 0; i < eventCount; i++)
			{
				hui::setInputEvent(hui::getInputEventAt(i));
				doFrame(i == eventCount - 1);
			}
		}
		else
		{
			// if no events, just do the frame
			doFrame(true);
		}
	} while (!exitNow);

	hui::saveDockingState("layout.hui");
	hui::shutdown();

	return 0;
}
