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
	hui::color(hui::widgetIsPressed() ? hui::Color::red : hui::Color::white);
	hui::drawBorderedImage(inf.image, inf.border, hui::getWidgetRect());
	hui::lineStyle({ hui::Color::orange, 1.5f });
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
	// setup a Horus UI context, with given service providers
	hui::Settings settings;

	settings.providers.file = new hui::StdioFileProvider();
	settings.providers.fileDialogs = new hui::NativeFileDialogsProvider();
	settings.providers.font = new hui::FreetypeFontProvider();
	settings.providers.gfx = new hui::OpenGLGraphicsProvider();
	settings.providers.image = new hui::StbImageProvider();
	settings.providers.input = new hui::Sdl3InputProvider();
	settings.providers.rectPack = new hui::StbRectPackProvider();
	settings.providers.utf = new hui::UtfCppProvider();

	// create the context
	auto huiContext = hui::contextCreate(settings);
	hui::contextSet(huiContext); // set as current context

	// initialize SDL input provider
	hui::SdlInitParams sdlParams;

	sdlParams.vSync = false;
	hui::initializeSdl(sdlParams);

	// create the main window (this will also create a graphics (GL/VK/D3D/etc.) context)
	auto mainWnd = HORUS_INPUT->createWindow("Horus Example - Custom Widget", hui::NativeWindowFlags::Resizable, hui::NativeWindowState::Maximized, hui::Rect(0, 0, 1000, 800));

	// initialize the graphics API, since now we have a first window created
	// (we cant initialize the graphics api without a window)
	HORUS_GFX->initialize();

	// initialize the UI renderer for the current context
	hui::initializeRenderer();

	// load a theme
	const u32 errSize = 2048;
	char err[errSize] = { 0 };
	auto theme = hui::loadThemeFromJson("../themes/default.theme.json", err, errSize);

	if (!theme)
	{
		printf("Theme JSON error: %s\n", err);
		theme = hui::themeCreate(hui::contextGetSettings().defaultAtlasSize);

		// initialize all elements with white image
		hui::WidgetElementInfo defInfo;
		defInfo.image = hui::themeGetImage(theme, "__WHITEIMAGE__");
		defInfo.color = hui::Color::white;
		defInfo.textColor = hui::Color::white;

		for (u32 i = 0; i < (u32)hui::WidgetElementId::Count; i++)
		{
			for (u32 j = 0; j < (u32)hui::WidgetStateType::Count; j++)
			{
				hui::themeSetWidgetElement(theme, (hui::WidgetElementId)i, (hui::WidgetStateType)j, defInfo);
			}
		}
	}

	// set the current theme
	hui::themeSet(theme);

	// build the theme
	// after we load the theme and more images and fonts, we need to rebuild the theme (into the image atlas)
	hui::themeBuild(theme);

	hui::getThemeWidgetElementInfo(hui::WidgetElementId::BoxBody, hui::WidgetStateType::Normal, inf);

	// start the main loop
	bool exitNow = false;

	while (!exitNow)
	{
		// clear the main window as a test
		HORUS_INPUT->setCurrentWindow(mainWnd);
		glClearColor(0.4f, 0.4f, 0.4f, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		// get the events from SDL or whatever input provider is set, it will fill a queue of events
		hui::contextUpdate();

		// check the event count
		auto eventCount = hui::inputEventGetCount();

		// the main frame rendering and input handling
		auto doFrame = [&](bool lastEventInQueue)
		{
			hui::nativeWindowSetCurrent(mainWnd);
			hui::renderBegin();
			// begin an actual frame of the gui
			hui::frameBegin();
			// disable rendering if its not the last event in the queue
			// no need to render while handling all the input events
			// we only render on the last event in the queue
			hui::skipRenderingThisFrame(!lastEventInQueue);

			const int maxPts = 32;
			hui::Point pts[maxPts] = { 0 };
			u32 ptCount = 0;

			hui::Rect rc = {30, 30, 500, 400};

			hui::layoutBegin(rc);
			curveEditor(55, maxPts, pts, ptCount, hui::Color::red);
			hui::layoutEnd();

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

				if (hui::inputGetEvent().type == hui::InputEvent::Type::WindowClose)
				{
					if (hui::inputGetEvent().window == mainWnd)
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