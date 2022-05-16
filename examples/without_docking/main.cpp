#pragma execution_character_set("utf-8")
#include "horus.h"

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

	hui::SdlSettings sdlSettings;

	sdlSettings.mainWindowTitle = "HorusUI Example - Without docking";
	sdlSettings.mainWindowRect = { 0, 0, 1200, 1000 };

	hui::setupSDL(sdlSettings);
	HORUS_GFX->initialize(); // init the gfx objects, since we have now a graphics context set
	hui::initializeRenderer(); // init UI renderer for the current context

	char err[2048];
	auto theme = hui::loadThemeFromJson("../themes/default.theme.json", err, 2048);

	if (!theme)
	{
		printf(err);
		exit(1);
	}

	auto largeFnt = hui::getThemeFont(theme, "title");
	hui::setTheme(theme);
	bool exitNow = false;

	auto ico1 = hui::loadImage("../themes/icons/ic_attach_file_white_24dp.png");
	auto ico2 = hui::loadImage("../themes/icons/ic_attach_money_white_24dp.png");
	auto ico3 = hui::loadImage("../themes/icons/ic_border_all_white_24dp.png");
	auto ico4 = hui::loadImage("../themes/icons/ic_border_inner_white_24dp.png");
	auto ico5 = hui::loadImage("../themes/icons/ic_border_outer_white_24dp.png");

	// after we load the theme and more images and fonts, we need to rebuild the theme (image atlas)
	hui::buildTheme(theme);

	//auto w = hui::createWindow("TEST", 600, 500, hui::WindowFlags::Resizable);

	while (!exitNow)
	{
		// get the events from SDL or whatever input provider is set, it will fill a queue of events
		hui::processInputEvents();
		// lets check the event count
		int eventCount = hui::getInputEventCount();

		// the main frame rendering and input handling
		auto doFrame = [&](bool lastFrame)
		{
			hui::setWindow(hui::getMainWindow());
			hui::beginWindow(hui::getMainWindow());
			hui::setDisableRendering(!lastFrame);

			if (lastFrame)
			{
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
			}

			// horus ui
			hui::beginFrame();
			hui::Rect panelRect = { 5, 5, 300, 500 };
			hui::WidgetElementInfo elemInfo;
			hui::getThemeWidgetElementInfo(hui::WidgetElementId::PopupBody, hui::WidgetStateType::Normal, elemInfo);
			hui::setColor(hui::Color::white);
			// draw before container, because it will clip our panel image (using padding)
			hui::drawBorderedImage(elemInfo.image, elemInfo.border, panelRect);

			hui::beginContainer(panelRect);
			hui::labelCustomFont("Information", largeFnt);
			hui::button("Activate shields");
			static bool chk1, chk2, chk3;
			hui::beginTwoColumns();
			chk1 = hui::check("Option 1", chk1);
			chk2 = hui::check("Option 2", chk2);
			hui::nextColumn();
			chk3 = hui::check("Option 3", chk3);
			hui::pushTint(hui::Color::cyan);
			
			if (hui::button("Browse..."))
			{
				char path[256] = {0};
				if (hui::openFileDialog("*.*", "", path, 256))
				{
					printf("Browsed for `%s`\n", path);
				}
			}

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

			hui::beginColumns(5);
			hui::pushWidth(0.5);
			hui::iconButton(ico1, 32);
			hui::popWidth();
			hui::nextColumn();
			hui::pushWidth(1);
			hui::iconButton(ico2, 32);
			hui::popWidth();
			hui::nextColumn();
			hui::iconButton(ico3, 32);
			hui::nextColumn();
			hui::iconButton(ico4, 32);
			hui::nextColumn();
			hui::iconButton(ico5, 32);
			hui::endColumns();
			
			hui::endContainer();
			hui::endFrame();
			hui::endWindow();

			if (lastFrame)
				hui::presentWindow(hui::getMainWindow());

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
	}

	hui::shutdown();

	return 0;
}
