#pragma execution_character_set("utf-8")
#include "horus.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <filesystem>

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

// Grab some image handles to use for the window icons
hui::HImage icon1, icon2, icon3, icon4, icon5, tabicon1, tabicon2, tabicon3, img;

void loadImages()
{
	// Grab some image handles to use for the window icons
	icon1 = hui::loadImage("../themes/icons/ic_attach_file_white_24dp.png");
	icon2 = hui::loadImage("../themes/icons/ic_attach_money_white_24dp.png");
	icon3 = hui::loadImage("../themes/icons/ic_border_all_white_24dp.png");
	icon4 = hui::loadImage("../themes/icons/ic_border_inner_white_24dp.png");
	icon5 = hui::loadImage("../themes/icons/ic_border_outer_white_24dp.png");
	tabicon1 = hui::loadImage("../themes/icons/icons8-equivalent-20.png");
	tabicon2 = hui::loadImage("../themes/icons/icons8-settings-20.png");
	tabicon3 = hui::loadImage("../themes/icons/icons8-opened-folder-20.png");
	img = hui::loadImage("../themes/default/lena.png");
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
	settings.dockNodeSpacing = 3;
	settings.dockNodeResizeSplitterHitSize = 8;
	//settings.dockingStyle = hui::DockingGuidesStyle::InsideNativeWindows;
	//settings.dockNodeDockingSizeRatio = 0.33f;

	// Create the context
	auto huiContext = hui::createContext(settings);
	hui::setContext(huiContext); // set as current context

	// Initialize SDL input provider
	hui::SdlInitParams sdlParams;

	sdlParams.vSync = false;
	hui::initializeSdl(sdlParams);

	// Create the main window (this will also create a graphics (GL/VK/D3D/etc.) context)
	auto mainWnd = HORUS_INPUT->createWindow("HorusUI Widget Examples", hui::NativeWindowFlags::Resizable, hui::NativeWindowState::Maximized, hui::Rect(0, 0, 1500, 800));

	// Create a main dock node for the main window, so we can dock windows in there
	hui::DockNodeId mainDockNode = hui::createRootDockNode(mainWnd);

	// Create the docking layout by splitting dock nodes around
	{
		hui::DockNodeId n1, n2;
		hui::dockLayoutSplit(mainDockNode, hui::DockNodeSplitType::Left, 0.5f, &n1, &n2 );
		hui::dockLayoutSetNodeWindow(n1, "hui");
		hui::dockLayoutSetNodeWindow(n2, "scene");
		hui::dockLayoutSplit(mainDockNode, hui::DockNodeSplitType::Top, 0.5f, &n1, &n2);
		hui::dockLayoutSetNodeWindow(n2, "inspector");
		hui::dockLayoutRecalculate();
	}

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

	// Grab a font handle from the theme to use later
	auto largeFnt = hui::getThemeFont(theme, "title");

	// Set the current theme
	hui::setTheme(theme);

	loadImages();

	// Build the theme
	// After we load the theme and more images and fonts, we need to rebuild the theme (into the image atlas)
	hui::buildTheme(theme);


	// Start the main loop
	bool exitNow = false;

	while (!exitNow)
	{
		// Clear the main window as a test
		HORUS_INPUT->setCurrentWindow(mainWnd);
		glClearColor(1, 1, 0, 1);
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
				largeFnt = hui::getThemeFont(theme, "title");
				loadImages();
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

		// Check the event count
		auto eventCount = hui::getInputEventCount();

		// the main frame rendering and input handling
		auto doFrame = [&](bool lastEventInQueue)
		{
			static bool confineSceneToWindow = false;

			auto userDrawing = [](hui::HNativeWindow wnd)
			{
				auto nativeWndSize = HORUS_INPUT->getWindowSize(wnd);
				hui::Rect rc;

				if (confineSceneToWindow)
				{
					rc = hui::getWindowClientRect("scene");
				}
				else
				{
					rc = {0, 0, nativeWndSize.x, nativeWndSize.y};
				}

				static f32 x = 1;
				static f32 t = 1;
				i32 vp[4];

				glClearColor(0,.4,0,1);
				glClear(GL_COLOR_BUFFER_BIT);
				glGetIntegerv(GL_VIEWPORT, vp);
				glViewport(rc.x, nativeWndSize.y - rc.bottom(), rc.width, rc.height);
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(0, 1, 0, 1, -1, 1);
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
				t += hui::getFrameDeltaTime();
				glViewport(vp[0], vp[1], vp[2], vp[3]);
			};

			// Begin an actual frame of the gui
			hui::beginFrame();
			// disable rendering if its not the last event in the queue
			// no need to render while handling all the input events
			// we only render on the last event in the queue
			hui::setDisableRendering(!lastEventInQueue);

			if (0&&hui::beginWindow("hui", "HUI", nullptr, tabicon1))
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
					hui::setWindowVisible("ui", true);
				static bool chk1, chk2, chk3;
				/*hui::beginTwoColumns();
				hui::check("Option 1", &chk1);
				hui::check("Option 2", &chk2);
				hui::nextColumn();
				hui::check("Option 3", &chk3);
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
					hui::setWindowVisible("ui", true);
				}

				hui::popTint();
				hui::endColumns();*/
				static float val;
				hui::sliderFloat("slider1", 0, 100, val);
				static char txt[2000];
				hui::textInput(txt, 2000, hui::TextInputValueMode::Any, "Write something here");
				hui::space();

				static f32 scrollPos = 0;
				hui::beginScrollView("scrollView1", 500, scrollPos);

				//hui::pushSpacing(500);
				static hui::Color col1 = hui::Color(3,0,0,1);
				static hui::Color col2 = hui::Color::blue;
				hui::colorPicker("cp1",  &col1, hui::ColorPickerFlags(0), &col2);
				//hui::colorPicker("cp2", &col2);
				//hui::popSpacing();

				hui::beginSameLine();

				hui::label("Text here", hui::HAlignType::Right);
				hui::label("Text here", hui::HAlignType::Left);
				hui::button("Button1");
				hui::button("Button2");
				hui::button("Button3");

				static i32 ddIndex = 0;
				static const char* items[10] = {
					"Item 01",
					"Item 02",
					"Item 03",
					"Item 04",
					"Item 05 sd fasdf asfasdfa",
					"Item 06",
					"Item 07",
					"Item 08",
					"Item 09",
					"Item 10",
				};

				hui::setNextWidth(150);
				hui::dropdown("dd", ddIndex, items, 10,6);

				//hui::setNextWidth(100);
				hui::comboSliderFloat(&scrollPos);
				hui::label("Text here", hui::HAlignType::Left);
				hui::endSameLine();

				hui::beginSameLine();
				hui::setNextWidth(0.33333f);
				hui::button("Action1");
				hui::setNextWidth(0.33333f);
				hui::button("Action2");
				hui::setNextWidth(0.33333f);
				hui::button("Action3");
				hui::endSameLine();

				hui::beginSameLine();
				hui::setNextWidth(0.1f);
				hui::check("Check01", &chk1);
				hui::setNextWidth(0.1f);
				hui::check("Check02", &chk2);
				hui::setNextWidth(0.5f);
				hui::check("Check03", &chk3);
				hui::endSameLine();


				static i32 rdoVal = 0;
				hui::beginSameLine();
				hui::setNextWidth(0.1f);
				hui::radio("Check01", &rdoVal, 0);
				hui::setNextWidth(0.1f);
				hui::radio("Check02", &rdoVal, 1);
				hui::setNextWidth(0.5f);
				hui::radio("Check03", &rdoVal, 2);
				hui::endSameLine();

				static i32 ival = 0;

				hui::comboSliderIntegerRanged(&ival, 0, 100, 0.01f, 1, "VAL: %.0f");

				hui::beginSameLine();
				hui::pushSameLineSpacing(0);
				hui::setNextWidth(0.25f);
				hui::image(img, 150, hui::HAlignType::Center);
				hui::setNextWidth(0.25f);
				hui::image(img, 50, hui::HAlignType::Center);
				hui::setNextWidth(0.25f);
				hui::image(img, 50, hui::HAlignType::Center);
				//hui::setNextWidth(0.25f);
				hui::pushWidgetStyle(hui::WidgetType::ImageButton, "important");
				hui::imageButton(tabicon3, 50, 50);
				static bool down = false;
				if (hui::imageButton(tabicon3, 50, 50, 0, down))
				{
					down = !down;
				}

				static bool showpop = false;

				if (hui::imageButton(tabicon3, 50, 50, 0, showpop))
				{
					showpop = !showpop;
				}

				if (showpop)
				{
					hui::beginPopup("imagebtnpop", 500);
					hui::label("Hello from popup!");

					hui::beginSameLine();
					hui::setNextWidth(0.5);

					if (hui::mustClosePopup() || hui::button("Close Popup"))
					{
						hui::closePopup();
						showpop = false;
					}

					hui::space(5);
					//hui::setNextWidth(0.5f);
					hui::button("Another Action");

					hui::endSameLine();
					hui::line();
					hui::label("sdf sdf asdf adsfasd");
					hui::label("sdf sdf asdf adsfasd");
					hui::label("sdf sdf asdf adsfasd");
					hui::label("sdf sdf asdf adsfasd");

					hui::endPopup();
				}

				hui::popWidgetStyle();
				hui::popSameLineSpacing();
				hui::endSameLine();
				hui::line();
				hui::labelMultiline("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur?", hui::HAlignType::Left);
				hui::line();
				hui::button("I AGREE Long text Label for this button to see ellipsis");
				hui::line();
				scrollPos = hui::endScrollView();
				hui::pushTint(hui::Color::orange);
				if (hui::button("Exit"))
					exitNow = true;
				hui::popTint();
				/*
				hui::beginColumns(5);
				hui::pushWidth(0.5);
				hui::imageButton(icon1, 32);
				hui::popWidth();
				hui::nextColumn();
				hui::pushWidth(1);
				hui::imageButton(icon2, 32);
				hui::popWidth();
				hui::nextColumn();
				hui::imageButton(icon3, 32);
				hui::nextColumn();
				hui::imageButton(icon4, 32);
				hui::nextColumn();
				hui::imageButton(icon5, 32);
				hui::endColumns();
				*/
				//hui::endContainer();
				hui::endWindow();
			}

			// start to add widgets in the window
			if (hui::beginWindow("inspector", "Inspector", nullptr, tabicon2))
			{
				hui::endWindow();
			}

			hui::setNextWindowFlags(hui::WindowFlags::Transparent);

			if (hui::beginWindow("scene", "Scene", nullptr, tabicon3))
			{
				if (lastEventInQueue)
				{
					hui::addRenderCallback(userDrawing);
				}

				static bool confine = false;

				if (hui::check("Confine scene to this window rectangle", &confine))
				{
					confineSceneToWindow = confine;
				}

				hui::endWindow();
			}

			if (hui::beginWindow("hui", "Widget Examples", nullptr, tabicon3))
			{
				static f32 scroller = 0;

				hui::beginScrollView("scrl1", 0, scroller);

				static bool listSelection[5] = {false};
				static const char* listItems[] = { "Apple", "Banana", "Cherry", "Date", "Elderberry" };
				hui::label("List Box:");
				hui::list("myList", listSelection, hui::ListSelectionMode::Multiple, listItems,5, 78);

				hui::space();
				hui::label("Table Widget:");
				hui::pushSpacing(0);
				hui::pushWidgetPadding(0);
				if (hui::beginTable("myTable", 4, 0, hui::TableFlags::BordersH | hui::TableFlags::None | hui::TableFlags::AltRowBg | hui::TableFlags::Resizable | hui::TableFlags::Stretch))
				{
					hui::startHeader();
					hui::setupColumn(0, 0);
					hui::setupColumn(1, 110, hui::TableColumnFlags::FixedResize);
					hui::setupColumn(2, 0);
					hui::setupColumn(3, 0);

					hui::label("Column 1", hui::HAlignType::Center);
					hui::nextCell();
					hui::label("Column 2", hui::HAlignType::Center);
					hui::nextCell();
					hui::button("Column 3");
					hui::nextCell();
					hui::label("Column 4", hui::HAlignType::Center);

					hui::nextRow();
					hui::setCellColumnSpan(3); // span 2 columns
					hui::label("Row 1, Cell 1 wdf dfasfasf asdf asf sadf asdf asfasf asf asf ");
					hui::nextCell();
					hui::setCellColor(hui::Color::red);
					hui::label("AOAKAOAO1");
					hui::label("AOAKAOAO2");
					hui::label("AOAKAOAO3");
					hui::label("AOAKAOAO4");

					hui::nextRow();
					hui::setRowColor(hui::Color::blue);
					hui::setCellColumnSpan(2); // Make this cell span 2 columns
					hui::label("Row 2, Cell 1\n(Multi-line)");
					hui::button("Tall Button");
					hui::nextCell();
					// Skip cell 2 since previous cell spanned it
					hui::label("Row 2, Cell 3 (prev cols spanned)");
					hui::nextCell();
					hui::label("Row 2, Cell 3 (prev cols spanned)");

					hui::nextRow();
					hui::label("Row 3, Cell 1");
					hui::nextCell();
					// Nested Table
					hui::label("Nested Table:");
					if (1&&hui::beginTable("nestedTable", 2, 0, hui::TableFlags::Borders | hui::TableFlags::AltRowBg|hui::TableFlags::Stretch|hui::TableFlags::Resizable))
					{
						hui::startHeader();
						hui::label("Sub 1");
						hui::nextCell();
						hui::label("Sub 2");

						hui::nextRow();
						hui::label("A");
						hui::nextCell();
						hui::label("B");

						hui::nextRow();
						hui::label("C");
						hui::nextCell();
						hui::label("D");

						hui::endTable();
					}
					hui::nextCell();
					hui::label("Row 3, Cell 3");

					hui::nextCell();
					hui::label("Row 3, Cell 3");

					for (int k = 0; k < 4; k++)
					{
						hui::nextRow();
						hui::label("Row 4, Cell 1");
						hui::nextCell();
						hui::label("Row 4, Cell 2");
						hui::nextCell();
						hui::label("Row 4, Cell 3");
						hui::nextCell();
						hui::label("Row 4, Cell 4");
					}

					hui::nextRow();
					hui::button("Row 3, Cell 3");
					hui::nextCell();
					hui::label("Row 3, Cell 3");
					hui::nextCell();
					hui::label("Row 3, Cell 3");
					hui::nextCell();
					hui::label("Row jgW3, Cell 3");

					hui::endTable();
					hui::popWidgetPadding();
					hui::popSpacing();

					scroller = hui::endScrollView();
				}

				hui::endWindow();
			}

			hui::endFrame();

			if (lastEventInQueue)
				hui::present();
		};

		// if we have events, then go through all of them and call the frame render and input
		if (eventCount)
		{
			for (int i = 0; i < eventCount; i++)
			{
				hui::setInputEvent(hui::getInputEventAt(i));

				if (hui::getInputEvent().type == hui::InputEvent::Type::Key &&
					hui::getInputEvent().key.code == hui::KeyCode::F2 &&
					hui::getInputEvent().key.down)
				{
					reloadTheme();
				}

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
