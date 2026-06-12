#pragma execution_character_set("utf-8")
#include "horus.h"

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

static const char* themeFilePath = "../themes/default.theme.json";
static std::filesystem::file_time_type themeLastWriteTime;
static bool themeNeedsReload = false;

static void reloadTheme(hui::Sdl3GfxApi gfxApi)
{
	char err[2048] = { 0 };
	auto newTheme = hui::loadThemeFromJson(themeFilePath, err, sizeof(err));

	if (!newTheme)
	{
		printf("Theme reload error: %s\n", err);
		return;
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

	auto theme = hui::loadThemeFromJson(themeFilePath, err, errSize);

	if (!theme)
	{
		printf("Theme JSON error: %s\n", err);
		theme = hui::themeCreate(hui::contextGetSettings().defaultAtlasSize);

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

	hui::themeSet(theme);
	hui::themeBuild(theme);
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
		}

		auto doFrame = [&](bool lastEventInQueue)
		{
			hui::frameBegin();
			hui::skipRenderingThisFrame(!lastEventInQueue);

			if (hui::windowBegin("demo", "Horus Demo"))
			{
				hui::showDemo();
				hui::windowEnd();
			}

			hui::frameEnd();

			if (lastEventInQueue)
				hui::present();
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
