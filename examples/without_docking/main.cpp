#pragma execution_character_set("utf-8")
#include "horus.h"

#define _USE_MATH_DEFINES
#include <cmath>

// backends
#include "sdl_input_provider.h"
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
	settings.dockNodeSpacing = 3;
	//settings.dockNodeResizeSplitterHitSize = 6;
	//settings.dockNodeDockingSizeRatio = 0.33f;
	
	//1. Create the context
	auto ctx = hui::createContext(settings);
	hui::setContext(ctx); // set as current context

	
	hui::SdlInitParams sdlParams;


	sdlParams.vSync = false;

	//2. Initialize SDL input provider
	hui::initializeSdl(sdlParams);

	//3. Create the main window (this will also create a graphics (GL/VK/D3D/etc.) context)
	auto mainWnd = HORUS_INPUT->createWindow("Horus Examples", hui::OsWindowFlags::Resizable, hui::OsWindowState::Maximized, hui::Rect(0, 0, 1500, 800));

	hui::DockNodeId mainDockNode = hui::createRootDockNode(mainWnd);

	if(1)
	{
		hui::DockNodeId n1, n2;
		hui::dockLayoutSplit(mainDockNode, hui::DockNodeSplitType::Left, 0.5f, &n1, &n2 );
		hui::dockLayoutSetNodeWindow(n1, "ui");
		hui::dockLayoutSetNodeWindow(n2, "inspector");
		auto inspectorNodeId = n2;
	
		hui::dockLayoutSplit(mainDockNode, hui::DockNodeSplitType::Top, 0.25f, &n1, &n2);
		hui::dockLayoutSetNodeWindow(n2, "hui");

		hui::dockLayoutSplit(inspectorNodeId, hui::DockNodeSplitType::Right, 0.25f, &n1, &n2);

		hui::dockLayoutSetNodeWindow(n1, "scene");

		hui::dockLayoutRecalculate();
	}

	//4. Initialize the graphics API, since now we have a first window created
	// (we cant initialize the graphics api without a window)
	HORUS_GFX->initialize();

	//5. Initialize the UI renderer for the current context
	hui::initializeRenderer();

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

	auto icon1 = hui::loadImage("../themes/icons/ic_attach_file_white_24dp.png");
	auto icon2 = hui::loadImage("../themes/icons/ic_attach_money_white_24dp.png");
	auto icon3 = hui::loadImage("../themes/icons/ic_border_all_white_24dp.png");
	auto icon4 = hui::loadImage("../themes/icons/ic_border_inner_white_24dp.png");
	auto icon5 = hui::loadImage("../themes/icons/ic_border_outer_white_24dp.png");
	auto tabicon1 = hui::loadImage("../themes/icons/icons8-equivalent-20.png");
	auto tabicon2 = hui::loadImage("../themes/icons/icons8-settings-20.png");
	auto tabicon3 = hui::loadImage("../themes/icons/icons8-opened-folder-20.png");

	// after we load the theme and more images and fonts, we need to rebuild the theme (image atlas)
	hui::buildTheme(theme);

	while (!exitNow)
	{
		// get the events from SDL or whatever input provider is set, it will fill a queue of events
		hui::processInputEvents();
		// lets check the event count
		int eventCount = hui::getInputEventCount();

		// the main frame rendering and input handling
		auto doFrame = [&](bool lastEventInQueue)
		{
			static bool confineSceneToWindow = false;

			auto tritri = [](hui::HOsWindow wnd)
			{
				auto osWndSize = HORUS_INPUT->getWindowClientSize(wnd);
				hui::Rect rc;

				if (confineSceneToWindow)
				{
					rc = hui::getWindowClientRect("scene");
				}
				else
				{
					rc = {0, 0, osWndSize.x, osWndSize.y};
				}

				//hui::beginContainer(hui::Rect(0, 0, osWndSize.x, osWndSize.y));
				//auto rc = hui::beginViewport();
				// some user drawing code, a triangle
				static f32 x = 1;
				static f32 t = 1;
				i32 vp[4];
				glClearColor(0,.4,0,1);
				glClear(GL_COLOR_BUFFER_BIT);
				glGetIntegerv(GL_VIEWPORT, vp);
				glViewport(rc.x, osWndSize.y - rc.bottom(), rc.width, rc.height);
				// Setup projection using glOrtho
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				//glOrtho(0, rectWidth, 0, rectHeight, -1, 1);
				glOrtho(0, 1, 0, 1, -1, 1);

				// Setup modelview
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();

				glBegin(GL_TRIANGLES);

				f32 radius1 = 0;
				f32 radius2 = 0.5;
				f32 step = 2 * M_PI / 10.0f;

				for (f32 u = 0; u < 2 * M_PI; u += step)
				{
					glColor3f(.1, 0.1, 0.1);
					glVertex2f(0.5f + radius1 * sinf(u + t), 0.5f + radius1 * cosf(u + t));
					glColor3f(.8, .8, 0);
					glVertex2f(0.5f + radius2 * sinf(u + t), 0.5f + radius2 * cosf(u + t));
					glColor3f(.1, 0.1, .1);
					glVertex2f(0.5f + radius2 * sinf(u + step + t), 0.5f + radius2 * cosf(u + step + t));
				}

				glEnd();

				x = sinf(t);
				t += 0.01f;

				//hui::endViewport();
				//hui::endContainer();
				glViewport(vp[0], vp[1], vp[2], vp[3]);
			};

			// begin an actual frame of the gui

			

			hui::beginFrame();
			// disable rendering if its not the last event in the queue
			// no need to render while handling all the input events
			// we only render on the last event in the queue
			hui::setDisableRendering(!lastEventInQueue);

			if (hui::beginWindow("hui", "HUI", nullptr, tabicon1))
			{
				// lets first draw a rect with a theme, for the panel
				hui::Rect panelRect = { 5, 5, 300, 500 };
				hui::WidgetElementInfo elemInfo;
				hui::getThemeWidgetElementInfo(hui::WidgetElementId::PopupBody, hui::WidgetStateType::Normal, elemInfo);
				hui::setColor(hui::Color::white);
				// draw before the beginContainer, because it will clip our panel image (using padding)
				//hui::drawBorderedImage(elemInfo.image, elemInfo.border, panelRect);

				// begin a widget container (it doesnt draw anything, a container is a layouting rectangle)
				//hui::beginContainer(panelRect);
				hui::labelCustomFont("Information", largeFnt);
				
				if (hui::button("DEBUG TREE"))
				{
					hui::debugWindows();
				}

				if (hui::button("Show UI window"))
					hui::setWindowVisibility("ui", true);
				static bool chk1, chk2, chk3;
				hui::beginTwoColumns();
				chk1 = hui::check("Option 1", chk1);
				chk2 = hui::check("Option 2", chk2);
				hui::nextColumn();
				chk3 = hui::check("Option 3", chk3);
				hui::pushTint(hui::Color::cyan);

				if (hui::button("Browse..."))
				{
					char path[256] = { 0 };

					if (hui::openFileDialog("*.*;*.png;*.jpg", "", path, 256))
					{
						printf("Browsed for `%s`\n", path);
					}
				}

				if (hui::button("Show UI"))
				{
					hui::setWindowVisibility("ui", true);
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
				hui::line();
				scrollPos = hui::endScrollView();
				hui::pushTint(hui::Color::orange);
				if (hui::button("Exit"))
					exitNow = true;
				hui::popTint();

				hui::beginColumns(5);
				hui::pushWidth(0.5);
				hui::iconButton(icon1, 32);
				hui::popWidth();
				hui::nextColumn();
				hui::pushWidth(1);
				hui::iconButton(icon2, 32);
				hui::popWidth();
				hui::nextColumn();
				hui::iconButton(icon3, 32);
				hui::nextColumn();
				hui::iconButton(icon4, 32);
				hui::nextColumn();
				hui::iconButton(icon5, 32);
				hui::endColumns();

				//hui::endContainer();
				hui::endWindow();
			}
			
			// start to add widgets in the window
			if (hui::beginWindow("inspector", "Inspector", nullptr, tabicon2))
			{
				hui::labelCustomFont("SETTINGS AND STUFF", hui::getFont("large"));
				static char txt[2000] = "hui";
				
				hui::label("Dock Target");
				hui::textInput(txt, 2000, hui::TextInputValueMode::Any, "Write something here");
			
				hui::space();
				if (hui::button("Dock Left"))
				{
					hui::dockWindow("inspector", txt, hui::DockType::Left);
				}
				if (hui::button("Dock Right"))
				{
					hui::dockWindow("inspector", txt, hui::DockType::Right);
				}
				if (hui::button("Dock Top"))
				{
					hui::dockWindow("inspector", txt, hui::DockType::Top);
				}
				if (hui::button("Dock Bottom"))
				{
					hui::dockWindow("inspector", txt, hui::DockType::Bottom);
				}
				if (hui::button("Dock As tab"))
				{
					hui::dockWindow("inspector", txt, hui::DockType::AsTab);
				}
				if (hui::button("UnDock"))
				{
					hui::undockWindow("inspector", HORUS_INPUT->getAbsoluteMousePosition());
				}
				hui::endWindow();
			}
			

			// start to add widgets in the window
			if (hui::beginWindow("assets", "Assets", nullptr, tabicon3))
			{
				
				hui::labelCustomFont("ASSETS OF COURSE", hui::getFont("heading"));
				static char txt[2000] = "hui";

				hui::label("Dock Target");
				
				hui::textInput(txt, 2000, hui::TextInputValueMode::Any, "Write something here");

				hui::space();
				if (hui::button("Dock Left"))
				{
					hui::dockWindow("assets", txt, hui::DockType::Left);
				}
				if (hui::button("Dock Right"))
				{
					hui::dockWindow("assets", txt, hui::DockType::Right);
				}
				if (hui::button("Dock Top"))
				{
					hui::dockWindow("assets", txt, hui::DockType::Top);
				}
				if (hui::button("Dock Bottom"))
				{
					hui::dockWindow("assets", txt, hui::DockType::Bottom);
				}
				if (hui::button("Dock As tab"))
				{
					hui::dockWindow("assets", txt, hui::DockType::AsTab);
				}
				if (hui::button("UnDock"))
				{
					hui::dockWindow("assets", 0, hui::DockType::Floating);
				}
				hui::endWindow();
			}

			hui::setNextWindowFlags(hui::WindowFlags::Transparent);

			if (hui::beginWindow("scene", "Scene", nullptr, tabicon3))
			{
				if (lastEventInQueue)
				{
					hui::addRenderCallback(tritri);
				}

				static bool confine = false;
				
				if (hui::check("Confine scene to this window rectangle", confine))
				{
					confineSceneToWindow = confine;
				}

				hui::endWindow();
			}

			if (hui::beginWindow("ui", "UI", nullptr, tabicon3))
			{

				hui::labelCustomFont("UI", hui::getFont("heading"));
				static char txt[2000] = "hui";

				hui::label("Dock Target");

				hui::textInput(txt, 2000, hui::TextInputValueMode::Any, "Write something here");

				hui::space();
				if (hui::button("Dock Left"))
				{
					hui::dockWindow("ui", txt, hui::DockType::Left);
				}
				if (hui::button("Dock Right"))
				{
					hui::dockWindow("ui", txt, hui::DockType::Right);
				}
				if (hui::button("Dock Top"))
				{
					hui::dockWindow("ui", txt, hui::DockType::Top);
				}
				if (hui::button("Dock Bottom"))
				{
					hui::dockWindow("ui", txt, hui::DockType::Bottom);
				}
				if (hui::button("Dock As tab"))
				{
					hui::dockWindow("ui", txt, hui::DockType::AsTab);
				}
				if (hui::button("UnDock"))
				{
					hui::dockWindow("ui", 0, hui::DockType::Floating);
				}
				hui::endWindow();
			}

			if (hui::beginWindow("log", "Log", nullptr, tabicon3))
			{
				hui::labelCustomFont("LOG", hui::getFont("heading"));
				static char txt[2000] = "hui";

				hui::label("Dock Target");

				hui::textInput(txt, 2000, hui::TextInputValueMode::Any, "Write something here");

				hui::space();
				if (hui::button("Dock Left"))
				{
					hui::dockWindow("log", txt, hui::DockType::Left);
				}
				if (hui::button("Dock Right"))
				{
					hui::dockWindow("log", txt, hui::DockType::Right);
				}
				if (hui::button("Dock Top"))
				{
					hui::dockWindow("log", txt, hui::DockType::Top);
				}
				if (hui::button("Dock Bottom"))
				{
					hui::dockWindow("log", txt, hui::DockType::Bottom);
				}
				if (hui::button("Dock As tab"))
				{
					hui::dockWindow("log", txt, hui::DockType::AsTab);
				}
				if (hui::button("UnDock"))
				{
					hui::dockWindow("log", 0, hui::DockType::Floating);
				}
				hui::endWindow();
			}

			hui::endFrame();
			
			if (lastEventInQueue)
				hui::present();
		};

		hui::setMouseCursor(hui::MouseCursorType::Arrow);
		// if we have events, then go through all of them and call the frame render and input
		if (eventCount)
		{
			for (int i = 0; i < eventCount; i++)
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

	return 0;
}
