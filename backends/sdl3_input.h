#pragma once
#include "horus.h"
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_system.h>
#include <vector>
#include <string>
#include <SDL3/SDL_vulkan.h>
#include "vulkan_graphics.h"
#ifdef _WINDOWS
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#endif

namespace hui
{
enum class Sdl3GfxApi
{
	OpenGL,
	DX11,
	DX12,
	Vulkan,
	Metal
};

struct SdlWindowProxy
{
	SDL_Window* sdlWindow = nullptr;
#ifdef _WINDOWS
	// dx11
	void* dx11SwapChain = nullptr;
	void* dx11RTV = nullptr;
	// dx12
	void* dx12SwapChain = nullptr;
	void* dx12RTVHeap = nullptr;
	void* dx12RTV = nullptr;
	u32 dx12CurrentBackBuffer = 0;
#endif
	// Vulkan surface (if using Vulkan)
	VkSurfaceKHR surface = VK_NULL_HANDLE;
};

struct Sdl3InitParams
{
	Sdl3GfxApi gfxApi = Sdl3GfxApi::OpenGL;
	bool vSync = false;
	SDL_GLContext sdlGlContext = nullptr; // set to a valid SDL GL context
	bool initializeSdl = true; // set to false if you already initialized SDL
};

void initSdl3(Services& services, const Sdl3InitParams& params);
void shutdownSdl3(Services& services);
HUI_API f32 getSdl3DeltaTime();
}