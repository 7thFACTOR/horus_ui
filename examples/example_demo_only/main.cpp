#pragma execution_character_set("utf-8")
#include "horus.h"

#include <algorithm>
#include <cstring>
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

hui::OpenGLTexture texAtlasGL;
hui::Dx11Texture texAtlasDX11;
hui::Dx12Texture texAtlasDX12;
hui::VulkanTexture texAtlasVK;

static void uploadAtlasTexture(hui::Sdl3GfxApi gfxApi)
{
	switch (gfxApi)
	{
	case hui::Sdl3GfxApi::OpenGL:
		texAtlasGL.resize(hui::themeGetAtlasImageData().width, hui::themeGetAtlasImageData().height);
		texAtlasGL.updateData(hui::themeGetAtlasImageData().pixels);
		hui::themeSetAtlasTexture(texAtlasGL.getHandle());
		break;
	case hui::Sdl3GfxApi::DX11:
		texAtlasDX11.resize(hui::themeGetAtlasImageData().width, hui::themeGetAtlasImageData().height);
		texAtlasDX11.updateData(hui::themeGetAtlasImageData().pixels);
		hui::themeSetAtlasTexture(texAtlasDX11.getHandle());
		break;
	case hui::Sdl3GfxApi::DX12:
		texAtlasDX12.resize(hui::themeGetAtlasImageData().width, hui::themeGetAtlasImageData().height);
		texAtlasDX12.updateData(hui::themeGetAtlasImageData().pixels);
		hui::themeSetAtlasTexture(texAtlasDX12.getHandle());
		break;
	case hui::Sdl3GfxApi::Vulkan:
		texAtlasVK.resize(hui::themeGetAtlasImageData().width, hui::themeGetAtlasImageData().height);
		texAtlasVK.updateData(hui::themeGetAtlasImageData().pixels);
		hui::themeSetAtlasTexture(texAtlasVK.getHandle());
		break;
	}
}

static void setScale(f32 scale, hui::Sdl3GfxApi gfxApi)
{
	hui::scaleSet(scale);
	hui::themeBuild(hui::themeGet());
	uploadAtlasTexture(gfxApi);
}

static char themeFilePath[256] = "../themes/default.theme.json";
static std::filesystem::file_time_type themeLastWriteTime;
static bool themeNeedsReload = false;

static const char* themeNames[] = { "Builtin", "Default", "Flat", "Win94" };
static const char* themePaths[] = { nullptr, "../themes/default.theme.json", "../themes/flat.theme.json", "../themes/win94.theme.json" };
static i32 currentThemeIndex = 1;
static constexpr u32 themeCount = 4;
static hui::HTheme currentTheme = nullptr;

static void reloadTheme(hui::Sdl3GfxApi gfxApi)
{
	hui::HTheme newTheme = nullptr;

	if (themeFilePath[0])
	{
		char err[2048] = { 0 };
		newTheme = hui::loadThemeFromJson(themeFilePath, err, sizeof(err));

		if (!newTheme)
		{
			printf("Theme reload error: %s\n", err);
			return;
		}
	}
	else
	{
		newTheme = hui::createBuiltinTheme(hui::contextGetSettings().defaultAtlasSize);
	}

	auto oldTheme = hui::themeGet();
	hui::themeSet(newTheme);
	hui::themeBuild(newTheme);
	uploadAtlasTexture(gfxApi);

	if (oldTheme)
	{
		hui::themeDestroy(oldTheme);
		hui::themeSet(newTheme);
	}

	// Update the global theme variable to point to the new theme
	currentTheme = newTheme;

	printf("Theme reloaded\n");
}

int main(int argc, char** args)
{
	hui::Sdl3InitParams sdlParams;

	sdlParams.vSync = true;
	sdlParams.gfxApi = hui::Sdl3GfxApi::Vulkan;

	hui::Settings settings;

	settings.dockNodeSpacing = 3;
	settings.dockNodeResizeSplitterHitSize = 8;
	settings.fpsThrottleEnable = true;

	hui::initStdioFileIO(settings.services);
	hui::initFreetype(settings.services);
	hui::initSdl3(settings.services, sdlParams);
	hui::initStbRectPack(settings.services);
	hui::initUtf(settings.services);

	std::string gfxApiName;

	switch (sdlParams.gfxApi)
	{
	case hui::Sdl3GfxApi::OpenGL:
		gfxApiName = "OpenGL";
		hui::initOpenGL(settings.services);
		break;
	case hui::Sdl3GfxApi::DX11:
		gfxApiName = "DX11";
		hui::initDx11(settings.services);
		break;
	case hui::Sdl3GfxApi::DX12:
		gfxApiName = "DX12";
		hui::initDx12(settings.services);
		break;
	case hui::Sdl3GfxApi::Vulkan:
		gfxApiName = "Vulkan";
		hui::initVulkan(settings.services);
		break;
	}

	auto huiContext = hui::contextCreate(settings);
	hui::contextSet(huiContext);

	auto mainWnd = HUI_SERVICES.createWindow(
		(std::string("Horus Demo - ") + gfxApiName).c_str(),
		hui::NativeWindowFlags::Resizable,
		hui::NativeWindowState::Maximized,
		hui::Rect(0, 0, 1500, 800));

	const u32 errSize = 2048;
	char err[errSize] = { 0 };

	currentTheme = hui::loadThemeFromJson(themeFilePath, err, errSize);

	if (!currentTheme)
	{
		printf("Theme JSON error: %s\n", err);
		currentTheme = hui::themeCreate(hui::contextGetSettings().defaultAtlasSize);

		hui::WidgetElementInfo defInfo;
		defInfo.image = hui::themeGetImage(currentTheme, "__WHITEIMAGE__");
		defInfo.color = hui::Color::white;
		defInfo.textColor = hui::Color::white;

		for (u32 i = 0; i < (u32)hui::WidgetElementId::Count; i++)
		{
			for (u32 j = 0; j < (u32)hui::WidgetStateType::Count; j++)
			{
				hui::themeSetWidgetElement(currentTheme, (hui::WidgetElementId)i, (hui::WidgetStateType)j, defInfo);
			}
		}
	}

	hui::themeSet(currentTheme);
	hui::themeBuild(currentTheme);
	uploadAtlasTexture(sdlParams.gfxApi);
	try { themeLastWriteTime = std::filesystem::last_write_time(themeFilePath); } catch (...) {}

	bool exitNow = false;

	while (!exitNow)
	{
		try
		{
			auto currentTime = std::filesystem::last_write_time(themeFilePath);
			if (currentTime != themeLastWriteTime)
			{
				themeLastWriteTime = currentTime;
				themeNeedsReload = true;
			}
		}
		catch (...) {}

		if (themeNeedsReload)
		{
			themeNeedsReload = false;
			reloadTheme(sdlParams.gfxApi);
		}

		HUI_SERVICES.setCurrentWindow(mainWnd);
		HUI_SERVICES.clearBackbuffer({ 0.3f, 0.0f, 0.1f, 1 });
		hui::contextGetSettings().deltaTime = hui::getSdl3DeltaTime();
		hui::contextUpdate();

		// Check if theme atlas needs rebuilding (e.g. after dynamically adding images)
		if (hui::themeRebuildAtlas(currentTheme))
		{
			switch (sdlParams.gfxApi)
			{
			case hui::Sdl3GfxApi::OpenGL:
				texAtlasGL.updateData(hui::themeGetAtlasImageData().pixels);
				hui::themeSetAtlasTexture(texAtlasGL.getHandle());
				break;
			case hui::Sdl3GfxApi::DX11:
				texAtlasDX11.updateData(hui::themeGetAtlasImageData().pixels);
				hui::themeSetAtlasTexture(texAtlasDX11.getHandle());
				break;
			case hui::Sdl3GfxApi::DX12:
				texAtlasDX12.updateData(hui::themeGetAtlasImageData().pixels);
				hui::themeSetAtlasTexture(texAtlasDX12.getHandle());
				break;
			case hui::Sdl3GfxApi::Vulkan:
				texAtlasVK.updateData(hui::themeGetAtlasImageData().pixels);
				hui::themeSetAtlasTexture(texAtlasVK.getHandle());
				break;
			}
		}

		auto eventCount = hui::inputEventGetCount();

		for (int i = 0; i < eventCount; i++)
		{
			const auto& ev = hui::inputEventGetAtIndex(i);

			if (ev.type == hui::InputEvent::Type::WindowClose
				&& ev.window == mainWnd)
			{
				exitNow = true;
			}

			if (ev.type == hui::InputEvent::Type::Key
				&& ev.key.down
				&& ev.key.code == hui::KeyCode::F5)
			{
				themeNeedsReload = true;
			}

			if (ev.type == hui::InputEvent::Type::MouseWheel
				&& has(ev.mouse.modifiers, hui::KeyModifiers::Control))
			{
				f32 currentScale = hui::scaleGet();
				setScale(std::clamp(currentScale + ev.mouse.wheel.y * 0.1f, 0.5f, 4.0f), sdlParams.gfxApi);
			}
		}

		auto doFrame = [&](bool lastEventInQueue)
		{
			hui::nativeWindowSetCurrent(mainWnd);
			hui::renderBegin();
			hui::frameBegin();
			hui::skipRenderingThisFrame(!lastEventInQueue);

			auto windowSize = HUI_SERVICES.getWindowSize(mainWnd);
			hui::Rect mainRect = { 0, 0, windowSize.x, windowSize.y };
			hui::paddingPush(hui::PaddingType::Layout, 0);
			hui::layoutBegin(mainRect);
			hui::boxBegin("mainBg", hui::Color::white, hui::WidgetElementId::WindowBody);

			hui::label("Theme:");
			hui::sameLine();
		if (hui::dropdown("themeSelector", currentThemeIndex, themeNames, themeCount))
		{
			if (themePaths[currentThemeIndex])
				strcpy_s(themeFilePath, sizeof(themeFilePath), themePaths[currentThemeIndex]);
			else
				themeFilePath[0] = 0;

			themeNeedsReload = true;
		}
			hui::sameLine();
			hui::label("(or press F5 to reload current)");
			hui::space();
			std::string crtScaleStr = std::to_string(hui::scaleGet());

			hui::label((std::string("Current UI scale (Ctrl+Wheel to change): ") + crtScaleStr).c_str());
			hui::sameLine();
			if (hui::button("Reset Scale"))
			{
				setScale(1.0f, sdlParams.gfxApi);
			}

			hui::line();
			hui::space();

			hui::showDemo();
			hui::boxEnd();
			hui::layoutEnd();
			hui::paddingPop(hui::PaddingType::Layout);

			hui::frameEnd();
			hui::renderEnd();

			if (lastEventInQueue)
				hui::presentNativeWindow(mainWnd);
		};

		if (eventCount)
		{
			for (int i = 0; i < eventCount; i++)
			{
				hui::inputEventSet(hui::inputEventGetAtIndex(i));

				if (hui::inputEventGet().type == hui::InputEvent::Type::WindowClose
					&& hui::inputEventGet().window == mainWnd)
				{
					exitNow = true;
				}

				doFrame(i == eventCount - 1);
			}
		}
		else
		{
			doFrame(true);
		}
	}

	hui::contextDestroy(huiContext);

	hui::shutdownStdioFileIO(settings.services);
	hui::shutdownFreetype(settings.services);
	hui::shutdownSdl3(settings.services);
	hui::shutdownStbRectPack(settings.services);
	hui::shutdownUtf(settings.services);

	switch (sdlParams.gfxApi)
	{
	case hui::Sdl3GfxApi::OpenGL:
		hui::shutdownOpenGL(settings.services);
		break;
	case hui::Sdl3GfxApi::DX11:
		hui::shutdownDx11(settings.services);
		break;
	case hui::Sdl3GfxApi::DX12:
		hui::shutdownDx12(settings.services);
		break;
	case hui::Sdl3GfxApi::Vulkan:
		hui::shutdownVulkan(settings.services);
		break;
	}

	hui::shutdown();

	return 0;
}

