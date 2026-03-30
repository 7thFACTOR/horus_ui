#pragma execution_character_set("utf-8")
#include "horus.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <filesystem>

// backends
#include "sdl3_input.h"
#include "opengl_graphics.h"
#include "dx11_graphics.h"
#include "dx12_graphics.h"
#include "vulkan_graphics.h"
#include "json_theme_loader.h"
#include "stb_rectpack.h"
#include "freetype_fonts.h"
#include "native_file_dialogs.h"
#include "stdio_fileio.h"
#include "utfcpp.h"

// Grab some image handles to use for the window icons
hui::HImage icon1, icon2, icon3, icon4, icon5, tabicon1, tabicon2, tabicon3, img;
//hui::HTexture tex1, tex2;
hui::OpenGLTexture texAtlasGL;
hui::Dx11Texture texAtlasDX11;
hui::Dx12Texture texAtlasDX12;
hui::VulkanTexture texAtlasVK;

void loadImages()
{
	auto theme = hui::getTheme();
	// Grab some image handles to use for the window icons
	icon1 = hui::loadThemeImage(theme, "../themes/icons/ic_attach_file_white_24dp.png");
	icon2 = hui::loadThemeImage(theme, "../themes/icons/ic_attach_money_white_24dp.png");
	icon3 = hui::loadThemeImage(theme, "../themes/icons/ic_border_all_white_24dp.png");
	icon4 = hui::loadThemeImage(theme, "../themes/icons/ic_border_inner_white_24dp.png");
	icon5 = hui::loadThemeImage(theme, "../themes/icons/ic_border_outer_white_24dp.png");
	tabicon1 = hui::loadThemeImage(theme, "../themes/icons/icons8-equivalent-20.png");
	tabicon2 = hui::loadThemeImage(theme, "../themes/icons/icons8-settings-20.png");
	tabicon3 = hui::loadThemeImage(theme, "../themes/icons/icons8-opened-folder-20.png");
	img = hui::loadThemeImage(theme, "../themes/default/lena.png");
	//tex1 = hui::loadTexture("../themes/default/lena.png");
	//tex2 = hui::loadTexture("../themes/default/lena.png");
}

int main(int argc, char** args)
{
	// Initialize SDL input provider
	hui::Sdl3InitParams sdlParams;

	sdlParams.vSync = false;
	sdlParams.gfxApi = hui::Sdl3GfxApi::OpenGL;
	//sdlParams.gfxApi = hui::Sdl3GfxApi::DX11;
	//sdlParams.gfxApi = hui::Sdl3GfxApi::DX12;
	//sdlParams.gfxApi = hui::Sdl3GfxApi::Vulkan;

	// Setup a Horus UI context, with given service providers
	hui::Settings settings;

	settings.dockNodeSpacing = 3;
	settings.dockNodeResizeSplitterHitSize = 8;

	hui::initStdioFileIO(settings.services);
	hui::initFreetype(settings.services);
	hui::initSdl3(settings.services, sdlParams);
	hui::initStbRectPack(settings.services);
	hui::initUtf(settings.services);

	switch (sdlParams.gfxApi)
	{
	case hui::Sdl3GfxApi::OpenGL:
		hui::initOpenGL(settings.services);
		break;
	case hui::Sdl3GfxApi::DX11:
		hui::initDx11(settings.services);
		break;
	case hui::Sdl3GfxApi::DX12:
		hui::initDx12(settings.services);
		break;
	case hui::Sdl3GfxApi::Vulkan:
		hui::initVulkan(settings.services);
		break;
	}

	auto huiContext = hui::createContext(settings);
	hui::setContext(huiContext); // set as current context

	// Create the main window (this will also create a graphics (GL/VK/D3D/etc.) context)
	auto mainWnd = hui::getSettings().services.createWindow("HorusUI Widget Examples", hui::NativeWindowFlags::Resizable, hui::NativeWindowState::Maximized, hui::Rect(0, 0, 1500, 800));

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

	// Load a theme
	const u32 errSize = 2048;
	char err[errSize] = { 0 };

	// Theme file path
	static const char* themeFilePath = "../themes/default.theme.json";

	auto theme = hui::loadThemeFromJson(themeFilePath, err, errSize);

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

	switch (sdlParams.gfxApi)
	{
	case hui::Sdl3GfxApi::OpenGL:
		texAtlasGL.resize(hui::getThemeAtlasImageData().width, hui::getThemeAtlasImageData().height);
		texAtlasGL.updateData(hui::getThemeAtlasImageData().pixels);
		hui::setThemeAtlasTexture(texAtlasGL.getHandle());
		break;
	case hui::Sdl3GfxApi::DX11:
		texAtlasDX11.resize(hui::getThemeAtlasImageData().width, hui::getThemeAtlasImageData().height);
		texAtlasDX11.updateData(hui::getThemeAtlasImageData().pixels);
		hui::setThemeAtlasTexture(texAtlasDX11.getHandle());
		break;
	case hui::Sdl3GfxApi::DX12:
		texAtlasDX12.resize(hui::getThemeAtlasImageData().width, hui::getThemeAtlasImageData().height);
		texAtlasDX12.updateData(hui::getThemeAtlasImageData().pixels);
		hui::setThemeAtlasTexture(texAtlasDX12.getHandle());
		break;
	case hui::Sdl3GfxApi::Vulkan:
		texAtlasVK.resize(hui::getThemeAtlasImageData().width, hui::getThemeAtlasImageData().height);
		texAtlasVK.updateData(hui::getThemeAtlasImageData().pixels);
		hui::setThemeAtlasTexture(texAtlasVK.getHandle());
		break;
	}

	bool exitNow = false;
	f32 lastMs = 0;

	auto reloadTheme = [theme, sdlParams, &err, errSize, &largeFnt]()
		{
			hui::deleteTheme(theme);
			auto theme = hui::loadThemeFromJson(themeFilePath, err, errSize);
			hui::setTheme(theme);
			loadImages();
			largeFnt = hui::getThemeFont(theme, "title");
			hui::buildTheme(theme);

			switch (sdlParams.gfxApi)
			{
			case hui::Sdl3GfxApi::OpenGL:
				texAtlasGL.updateData(hui::getThemeAtlasImageData().pixels);
				hui::setThemeAtlasTexture(texAtlasGL.getHandle());
				break;
			case hui::Sdl3GfxApi::DX11:
				texAtlasDX11.updateData(hui::getThemeAtlasImageData().pixels);
				hui::setThemeAtlasTexture(texAtlasDX11.getHandle());
				break;
			case hui::Sdl3GfxApi::DX12:
				texAtlasDX12.updateData(hui::getThemeAtlasImageData().pixels);
				hui::setThemeAtlasTexture(texAtlasDX12.getHandle());
				break;
			case hui::Sdl3GfxApi::Vulkan:
				texAtlasVK.updateData(hui::getThemeAtlasImageData().pixels);
				hui::setThemeAtlasTexture(texAtlasVK.getHandle());
				break;
			default:
				break;
			}
		};


	while (!exitNow)
	{
		// Clear the main window as a test
		hui::getSettings().services.setCurrentWindow(mainWnd);
		hui::getSettings().services.clearBackbuffer({ 0.1f, 0.0f, 0.1f, 1 });

		// track theme file modification time for auto-reload
		static auto lastModTime = std::filesystem::last_write_time(themeFilePath);
		static f32 checkTimer = 0;

		hui::getSettings().deltaTime = hui::getSdl3DeltaTime();
		checkTimer += hui::getSettings().deltaTime;

		// check if theme file has been modified (every 1 second)
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
				// ignore filesystem errors
			}
		}

		// get the events from SDL or whatever input provider is set, it will fill a queue of events
		hui::update();

		// reload theme on F2 key press
		if (hui::getInputEvent().type == hui::InputEvent::Type::Key
			&& hui::getInputEvent().key.code == hui::KeyCode::F2
			&& hui::getInputEvent().key.down)
		{
			reloadTheme();
		}

		auto eventCount = hui::getInputEventCount();

		// the main frame rendering and input handling
		auto doFrame = [&](bool lastEventInQueue)
		{
			static bool confineSceneToWindow = false;

			auto userDrawing = [](hui::HNativeWindow wnd)
			{
				auto nativeWndSize = hui::getSettings().services.getWindowSize(wnd);
				hui::Rect rc;

				if (confineSceneToWindow)
				{
					rc = hui::getWindowClientRectById("scene");
				}
				else
				{
					rc = {0, 0, nativeWndSize.x, nativeWndSize.y};
				}

				static f32 x = 1;
				static f32 t = 1;
				i32 vp[4];

				//glClearColor(0,.4,0,1);
				//glClear(GL_COLOR_BUFFER_BIT);
				//glGetIntegerv(GL_VIEWPORT, vp);
				//glViewport(rc.x, nativeWndSize.y - rc.bottom(), rc.width, rc.height);
				//glMatrixMode(GL_PROJECTION);
				//glLoadIdentity();
				//glOrtho(0, 1, 0, 1, -1, 1);
				//glMatrixMode(GL_MODELVIEW);
				//glLoadIdentity();

				//glBegin(GL_TRIANGLES);

				//f32 radius1 = 0;
				//f32 radius2 = 0.5;
				//f32 step = 2 * M_PI / 10.0f;

				//for (f32 u = 0; u < 2 * M_PI; u += step)
				//{
				//	glColor3f(.1, 0.1, 0.1);
				//	glVertex2f(0.5f + radius1 * sinf(u + t), 0.5f + radius1 * cosf(u + t));
				//	glColor3f(.8, .8, 0);
				//	glVertex2f(0.5f + radius2 * sinf(u + t), 0.5f + radius2 * cosf(u + t));
				//	glColor3f(.1, 0.1, .1);
				//	glVertex2f(0.5f + radius2 * sinf(u + step + t), 0.5f + radius2 * cosf(u + step + t));
				//}

				//glEnd();

				//x = sinf(t);
				//t += hui::getSettings().deltaTime;
				//glViewport(vp[0], vp[1], vp[2], vp[3]);
			};

			// Begin an actual frame of the gui
			hui::beginFrame();
			// disable rendering if its not the last event in the queue
			// no need to render while handling all the input events
			// we only render on the last event in the queue
			hui::setDisableRendering(!lastEventInQueue);

			if (hui::beginWindow("hui2", "HUI", nullptr, tabicon1))
			{
				// lets first draw a rect with a theme, for the panel
				hui::Rect panelRect = { 5, 5, 300, 500 };
				hui::WidgetElementInfo elemInfo;
				hui::getThemeWidgetElementInfo(hui::WidgetElementId::PopupBody, hui::WidgetStateType::Normal, elemInfo);
				hui::rendererSetColor(hui::Color::white);
				// draw before the beginContainer, because it will clip our panel image (using padding)
				//hui::drawBorderedImage(elemInfo.image, elemInfo.border, panelRect);

				// begin a widget container (it doesnt draw anything, a container is a layouting rectangle)
				//hui::beginContainer(panelRect);
				hui::labelCustomFont("Information", largeFnt);
				hui::label("Frame MS: "); hui::sameLine();
				hui::label("TEST");
				//hui::label((std::to_string(lastMs) + "##rer").c_str());

				hui::label("Peak Frame MS: "); hui::sameLine();
				hui::label(std::to_string(hui::getPeakFrameTimeMs()).c_str());

				hui::label("Avg Frame MS: "); hui::sameLine();
				hui::label(std::to_string(hui::getAvgFrameTimeMs()).c_str());



				if (hui::button("DEBUG TREE PRINT"))
				{
					hui::debugPrintWindows();
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
				*/
				//hui::sliderFloat("slider1", 0, 100, val);
				static char txt[2000];
				hui::textInput("txt", txt, 2000, hui::TextInputFlags::None, "Write something here");
				hui::space();

				static hui::Point scrollPos = 0;
				hui::pushPadding(hui::PaddingType::ScrollView, hui::Point(0, 0));
				hui::beginScrollView("scrollView1", 200, scrollPos, 0, hui::ScrollViewFlags::None);
				hui::button("asdf asdf asdf asdf asdf asdf asdf asdf asdf asdf ad");
				//hui::pushSpacing(500);
				static hui::Color col1 = hui::Color(3,0,0,1);
				static hui::Color col2 = hui::Color::blue;
				hui::colorPicker("cp1",  &col1, hui::ColorPickerFlags(0), &col2);
				//hui::colorPicker("cp2", &col2);
				//hui::popSpacing();
				hui::beginSameLineGroup(3);
				hui::button("COKCO1");
				hui::nextSameLineGroupWidget();
				hui::button("COKCO2");
				hui::nextSameLineGroupWidget();
				hui::button("COKCO3");
				hui::endSameLineGroup();
				hui::setNextWidth(1);
				hui::button("COKCO33");

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
				//hui::dropdown("dd", ddIndex, items, 10,6);

				//hui::setNextWidth(100);
				hui::comboSliderFloat(&scrollPos.y);
				hui::label("Text here", hui::HAlignType::Left);

				hui::setNextWidth(0.33333f);
				hui::button("Action1");
				hui::setNextWidth(0.33333f);
				hui::button("Action2");
				hui::setNextWidth(0.33333f);
				hui::button("Action3");

				hui::setNextWidth(0.1f);
				hui::check("Check01", &chk1);
				hui::setNextWidth(0.1f);
				hui::check("Check02", &chk2);
				hui::setNextWidth(0.5f);
				hui::check("Check03", &chk3);


				static i32 rdoVal = 0;
				hui::setNextWidth(0.1f);
				hui::radio("Check01", &rdoVal, 0);
				hui::setNextWidth(0.1f);
				hui::radio("Check02", &rdoVal, 1);
				hui::setNextWidth(0.5f);
				hui::radio("Check03", &rdoVal, 2);

				static i32 ival = 0;

				hui::comboSliderIntegerRanged(&ival, 0, 100, 0.01f, 1, "VAL: %.0f");

				hui::setNextWidth(0.25f);
				hui::image(img, 150, hui::HAlignType::Center);
				hui::setNextWidth(0.25f);
				hui::image(img, 50, hui::HAlignType::Center);
				hui::setNextWidth(0.25f);
				hui::image(img, 50, hui::HAlignType::Center);
				//hui::setNextWidth(0.25f);
				hui::pushWidgetStyle(hui::WidgetType::ImageButton, "important");
				hui::imageButton(tabicon3, 50, 50); hui::sameLine();
				static bool down = false;
				if (hui::imageButton(tabicon3, 50, 50, 0, down))
				{
					down = !down;
				}
				hui::sameLine();
				static bool showpop = false;

				if (hui::imageButton(tabicon3, 50, 50, 0, showpop))
				{
					showpop = !showpop;
				}

				if (showpop)
				{
					hui::beginPopup("imagebtnpop", 500);
					hui::label("Hello from popup!");

					hui::setNextWidth(0.5);

					if (hui::mustClosePopup() || hui::button("Close Popup"))
					{
						hui::closePopup();
						showpop = false;
					}

					hui::space(5);
					//hui::setNextWidth(0.5f);
					hui::button("Another Action");

					hui::line();
					hui::label("sdf sdf asdf adsfasd");
					hui::label("sdf sdf asdf adsfasd");
					hui::label("sdf sdf asdf adsfasd");
					hui::label("sdf sdf asdf adsfasd");

					hui::endPopup();
				}

				hui::popWidgetStyle();
				hui::line();
				hui::labelMultiline("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur?", hui::HAlignType::Left);
				hui::line();
				hui::button("I AGREE Long text Label for this button to see ellipsis");
				hui::line();
				scrollPos = hui::endScrollView();
				hui::popPadding(hui::PaddingType::ScrollView);

				static char strMulti[300000];
				static bool med = true;

				hui::check("med", &med);

				static hui::KeywordInfo kws[] = {
					{"function", hui::Color::orange},
					{"end", hui::Color::orange },
					{"local", hui::Color::green},
					{"(", hui::Color::blue, hui::KeywordInfo::Type::Delimiter},
					{")", hui::Color::blue, hui::KeywordInfo::Type::Delimiter}
				};

				static hui::RangeHighlight rh[] = {
					{ "\"", "\"", hui::Color::red, "\\"},
					{ "'", "'", hui::Color::cyan},
					{ "--", 0, hui::Color::lightGray},
					{ "//", 0, hui::Color::lightGray},
					{ "/*", "*/", hui::Color::green}
				};

				if (med)
					hui::multilineTextInput("mti", strMulti, 300000, 10/*, hui::MultilineTextInputFlags::LineNumbers*/,
						hui::MultilineTextInputFlags::LineNumbers
						| hui::MultilineTextInputFlags::HighlightCurrentLine
						|hui::MultilineTextInputFlags::WordWrap
						| hui::MultilineTextInputFlags::None
						, kws, 5, rh, 5);


				hui::pushTint(hui::Color::orange);
				hui::setNextWidth(1);
				hui::pushWidgetStyle(hui::WidgetType::Button, "important");
				if (hui::button("Exit"))
					exitNow = true;
				hui::popWidgetStyle();
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
					//hui::addRenderCallback(userDrawing);
				}

				static bool confine = false;

				if (hui::check("Confine scene to this window rectangle", &confine))
				{
					confineSceneToWindow = confine;
				}

				hui::endWindow();
			}

			if (1&&hui::beginWindow("hui", "Widget Examples", nullptr, tabicon3))
			{
				static f32 scroller = 0;

				//hui::beginScrollView("scrl1", 0, scroller, 0, hui::ScrollViewFlags::NoBorder);

				static bool listSelection[5] = {false};
				static const char* listItems[] = { "Apple", "Banana", "Cherry", "Date", "Elderberry" };
				hui::label("List Box:");
				//hui::list("myList", listSelection, hui::ListSelectionMode::Multiple, listItems,5, 78);

				hui::space();

				static char text[400] = { 0 };
				hui::textInput("SHEIDD", text, 400, hui::TextInputFlags::AutoSelectAll);

				hui::label("Table Widget:");
				hui::pushSpacing(0);
				hui::pushWidgetPadding(0);
				if (hui::beginTable("myTable", 4, 440, hui::TableFlags::Borders | hui::TableFlags::None | hui::TableFlags::AltRowBg | hui::TableFlags::Resizable | hui::TableFlags::Stretch))
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
					hui::label("Column 3", hui::HAlignType::Center);

					hui::nextCell();
					hui::label("Column 4", hui::HAlignType::Center);

					

					//// Virtualized rows: 10000 rows using VirtualScrollInfo
					//static hui::VirtualScrollInfo vtableInfo(10000); // 10k rows
					//// Initialize virtual list inside the table body
					//static hui::Point scrollPos = 0;
					//hui::beginScrollView("##tableScrollView", 440, scrollPos, 1000.0f, hui::ScrollViewFlags::NoBorder);

					//hui::beginVirtualListContent(vtableInfo);

					//// render the visible slice
					//while (vtableInfo.nextStep())
					//{
					//	if (vtableInfo.startIndex <= vtableInfo.endIndex)
					//	{
					//		for (u32 k = vtableInfo.startIndex; k <= vtableInfo.endIndex; ++k)
					//		{
					for (u32 k = 0; k < 100; ++k)
					{
						hui::nextRow();
						auto is = std::to_string(k);
						hui::label(is.c_str());
						hui::sameLine();
						hui::button(("Btn " + is).c_str());
						hui::nextCell();
						static char col2[100] = { 0 };
						hui::pushId((int)k);
						hui::setNextWidth(100);
						hui::textInput(("ed" + is).c_str(), col2, 100);
						hui::sameLine();
						hui::button(("Remove##" + is).c_str());
						hui::sameLine();
						hui::button(("Clone##" + is).c_str()); hui::sameLine();
						static bool chk = false;
						static i32 rad = 0;
						hui::check(("Chk##" + is).c_str(), &chk);
						hui::sameLine();
						hui::radio(("Rad1i##" + is).c_str(), &rad, 0);
						hui::radio(("Rad2i##" + is).c_str(), &rad, 1);
						hui::popId();
						hui::nextCell();
						hui::label("Col 3");
						hui::nextCell();
						hui::label("Col 4");
					}
					//		}
					//	}
					//}

					/*hui::endVirtualListContent();
					scrollPos = hui::endScrollView();*/
					hui::endTable();
					hui::popWidgetPadding();
					hui::popSpacing();

					hui::label("End of tableo");

					//scroller = hui::endScrollView();
				}

				hui::endWindow();
			}

			// --- virtual list demo window (modified)
			if (hui::beginWindow("virtual_list", "Virtual List Demo", nullptr, tabicon1))
			{
				// persistent virtual list state: only provide item count here
				static hui::VirtualScrollInfo vinfo(10000); // 100k items
				static hui::Point vscroll = { 0, 0 };

				hui::label("Virtualized list example (100k buttons)");

				// begin scroll view (viewport height 300)
				f32 viewH = 300.0f;
				// NOTE: we no longer need to pass vertical content height here; beginVirtualListContent sets it.
				hui::beginScrollView("##virt_list_scroll", viewH, vscroll, hui::Point(0, 0), hui::ScrollViewFlags::None);

				// initialize virtual list content (this sets the scrollview virtual height)
				hui::beginVirtualListContent(vinfo);

				// Step loop (measures first item, then issues remaining range)
				while (vinfo.nextStep())
				{
					if (vinfo.startIndex <= vinfo.endIndex)
					{
						for (u32 i = vinfo.startIndex; i <= vinfo.endIndex; ++i)
						{
							char buf[64];
							snprintf(buf, sizeof(buf), "Item %06u", i);
							if (hui::button(buf))
							{
								printf("clicked virtual item %u\n", i);
							}
							//hui::space(22.0f);
						}
					}
				}

				hui::endVirtualListContent();
				vscroll = hui::endScrollView();

				// show small status
				char info[128];
				snprintf(info, sizeof(info), "visible: %u..%u (count=%u)  scrollY=%.1f",
					vinfo.startIndex,
					vinfo.endIndex,
					vinfo.endIndex - vinfo.startIndex,
					vinfo.scrollOffsetY);
				hui::space(6);
				hui::label(info);

				hui::endWindow();
			}

			hui::endFrame();

			lastMs = hui::getLastFrameTimeMs();

			if (lastEventInQueue)
				hui::present();
		};

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

	hui::deleteContext(huiContext);

	hui::shutdownStdioFileIO(settings.services);
	hui::shutdownFreetype(settings.services);
	hui::shutdownSdl3(settings.services);
	hui::shutdownStbRectPack(settings.services);
	hui::shutdownUtf(settings.services);
	//hui::shutdownOpenGL(settings.services);
	hui::shutdownDx11(settings.services);
	//hui::shutdownDx12(settings.services);
	//hui::shutdownVulkan(settings.services);

	hui::shutdown();

	return 0;
}
