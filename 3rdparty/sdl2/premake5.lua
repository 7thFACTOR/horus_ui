project "sdl2"
	kind "SharedLib"
	language "C"
	includedirs { 'include' }
	files {
		"src/**.c",
		"src/**.cc"
	}

	filter { "system:windows" }
		excludes {
			"src/main/**.*",
			"src/test/**.*",
			"src/video/qnx/**.c",
			"src/core/unix/**.c",
			"src/video/x11/**.c",
			"src/thread/pthread/**.c",
			"src/thread/generic/SDL_sysmutex.c",
			"src/thread/generic/SDL_syssem.c",
			"src/thread/generic/SDL_systhread.c",
			"src/thread/generic/SDL_systls.c",
		}

		defines {"SDL_DISABLE_WINDOWS_IME"}
		defines
		{
			"WIN32",
			"__WIN32__",
			"SDL_SHARED",
			"WIN32_LEAN_AND_MEAN",
			"VC_EXTRALEAN",
			"_CRT_SECURE_NO_WARNINGS",
		}

		links { "user32", "gdi32", "winmm", "imm32", "ole32", "oleaut32", "version", "uuid" }

	filter { "system:linux" }
		excludes {
			"src/main/**.*",
			"src/test/**.*",
			"src/video/qnx/**.c",
			"src/thread/generic/**.*",
			"src/haptic/windows/**.*",
			"src/loadso/dummy/**.*",
			"src/timer/dummy/**.*",
		}

		defines
		{
			"SDL_SHARED",
			"SDL_VIDEO_DRIVER_X11",
			"SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS",
			"SDL_VIDEO_OPENGL_GLX",
			"SDL_LOADSO_DLOPEN",
			"SDL_TIMER_UNIX",
			"WIN32_LEAN_AND_MEAN",
			"VC_EXTRALEAN",
			"_CRT_SECURE_NO_WARNINGS",
		}

		buildoptions {"-fPIC"}
		links {"X11", "Xi", "dl"}
	filter {}
	
	filter { "system:macosx" }
		files {
		"src/**.m"
		}
		excludes {
			"src/main/**.*",
			"src/test/**.*",
			"src/core/linux/**.*",
			"src/joystick/iphoneos/**.*",
			"src/video/qnx/**.c",
			"src/thread/generic/**.*",
			"src/haptic/windows/**.*",
			"src/loadso/dummy/**.*",
			"src/timer/dummy/**.*",
		}

		defines
		{
			"SDL_SHARED",
			"SDL_POWER_MACOSX",
			"SDL_VIDEO_DRIVER_COCOA",
			"SDL_VIDEO_OPENGL_GLX",
			"SDL_LOADSO_DLOPEN",
			"SDL_TIMER_UNIX",
			"SDL_FRAMEWORK_COCOA",
			"SDL_FRAMEWORK_CARBON",
			"SDL_FILESYSTEM_COCOA",
		}

		buildoptions {"-fPIC"}
		links {"ForceFeedback.framework", "CoreVideo.framework", "Cocoa.framework", "IOKit.framework", "Carbon.framework", "CoreAudio.framework", "AudioToolbox.framework", "dl"}
	filter {}

	configuration "Debug"
		defines 
		{
			"NDEBUG"
		}
		optimize "On"
		targetname "sdl2"

	configuration "Release"
		defines
		{
			"NDEBUG"
		}
		optimize "On"
		targetname "sdl2"
