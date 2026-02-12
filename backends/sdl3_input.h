#pragma once
#include "horus.h"
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_system.h>
#include <vector>
#include <string>

namespace hui
{
enum class Sdl3GfxApi
{
	OpenGL,
	Direct3D11,
	Direct3D12,
	Vulkan,
	Metal
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
HORUS_API f32 getSdl3DeltaTime();
}