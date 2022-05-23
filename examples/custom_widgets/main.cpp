#pragma execution_character_set("utf-8")
#include <horus.h>

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

hui::WidgetElementInfo inf;

void curveEditor(f32 height, u32 maxPoints, hui::Point* points, u32& pointCount, const hui::Color& lineColor)
{
	hui::beginCustomWidget(height);
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

	sdlSettings.mainWindowTitle = "HorusUI Example - Custom Widgets";
	sdlSettings.mainWindowRect = { 0, 0, 1200, 1000 };
	hui::setupSDL(sdlSettings);
	HORUS_GFX->initialize(); // init the gfx objects, since we have now a graphics context set
	hui::initializeRenderer(); // init UI renderer for the current context

	auto theme = hui::loadThemeFromJson("../themes/default.theme.json");

	hui::setTheme(theme);
	auto largeFnt = hui::getThemeFont(theme, "title");

	hui::getThemeWidgetElementInfo(hui::WidgetElementId::ButtonBody, hui::WidgetStateType::Normal, inf);
	bool exitNow = false;

	hui::setWindow(hui::getMainWindow());

	while (!exitNow)
	{
		// get the events from SDL or whatever input provider is set, it will fill a queue of events
		hui::processInputEvents();
		int eventCount = hui::getInputEventCount();

		// the main frame rendering and input handling

		auto doFrame = [&](bool lastFrame)
		{
			hui::beginWindow(hui::getMainWindow());
			hui::setDisableRendering(!lastFrame);
			hui::clearBackground();

			hui::beginFrame();
			hui::Rect panelRect = { 50, 50, 350, 500 };
			hui::beginContainer(panelRect);
			hui::WidgetElementInfo elemInfo;
			hui::getThemeWidgetElementInfo(hui::WidgetElementId::PopupBody, hui::WidgetStateType::Normal, elemInfo);

			const int maxPts = 32;
			hui::Point pts[maxPts] = { 0 };
			u32 ptCount = 0;

			curveEditor(55, maxPts, pts, ptCount, hui::Color::red);

			if (hui::button("Exit"))
				hui::quitApplication();

			hui::endContainer();
			hui::endFrame();
			hui::endWindow();
			
			if (lastFrame)
				hui::presentWindow(hui::getMainWindow());

			if (hui::wantsToQuit() || hui::mustQuit())
				exitNow = true;
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
