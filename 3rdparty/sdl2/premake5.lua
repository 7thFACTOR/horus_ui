project "sdl2"
	kind "SharedLib"
	language "C"

	files {
		"src/**.c",
		"src/**.cc"
	}
	excludes {
		"src/main/**",
		"src/test/**",
		"**/qnx/**",
		"**/pandora/**",
		"**/raspberry/**",
		"**/vivante/**",
		"**/haiku/**",
		"**/psp/**",
		"**/android/**",
		"**/iphoneos/**",
		"**/emscripten/**",
		"**/nacl/**",
	}

	defines "SDL_SHARED"
	shared.includedirs "include"

	-- can't use filters to exclude files
	if os.target() == "windows" then
		excludes {
			"src/thread/generic/SDL_sysmutex.c",
			"src/thread/generic/SDL_syssem.c",
			"src/thread/generic/SDL_systhread.c",
			"src/thread/generic/SDL_systls.c",
			"**/pthread/**",
			"**/unix/**",
			"**/*bsd/**",
			"**/linux/**",
			"**/macosx/**",
			"**/darwin/**",
			"**/cocoa/**",
			"**/x11/**",
			"**/directfb/**",
			"**/kmsdrm/**",
			"**/mir/**",
			"**/wayland/**",
			"**/alsa/**",
			"**/jack/**",
			"**/pulseaudio/**",
		}
		links { "winmm", "imm32", "version", "kernel32", "vcruntime", "ucrtd" }
	elseif os.target() == "linux" or os.target() == "macosx" then
		excludes {
			"**/windows/**",
			"src/thread/generic/**",
			"src/loadso/dummy/**",
			"src/timer/dummy/**",
		}

		if os.target() == "linux" then
			shared.defines "__LINUX__"
			links {	"X11 `pkg-config --libs gtk+-3.0`",	"Xi", "dl" }
		else
			files "src/**.m"
			excludes "src/core/linux/**"
			shared.defines "__APPLE__"
			links {
				"ForceFeedback.framework", "CoreVideo.framework", "Cocoa.framework", "IOKit.framework", "Carbon.framework", "CoreAudio.framework", "AudioToolbox.framework", "dl"
			}
		end
	end

	filter "system:windows"
		staticruntime "On"
		defines {
			"WIN32",
			"__WIN32__",
			"_WINDOWS",
			"VC_EXTRALEAN",
			"_CRT_SECURE_NO_WARNINGS",
			"SDL_DISABLE_WINDOWS_IME"
		}

	filter "system:linux or macosx"
		buildoptions "-fPIC"
		defines {
			"SDL_VIDEO_OPENGL_GLX",
			"SDL_TIMER_UNIX",
			"SDL_LOADSO_DLOPEN",
		}

	filter "system:linux"
		linkgroups 'On'
		buildoptions "`pkg-config --cflags gtk+-3.0`"
		defines {
			"SDL_VIDEO_DRIVER_X11",
			"SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS",
			"WIN32_LEAN_AND_MEAN",
			"VC_EXTRALEAN",
			"_CRT_SECURE_NO_WARNINGS",
		}

	filter "system:macosx"
		defines {
			"SDL_POWER_MACOSX",
			"SDL_VIDEO_DRIVER_COCOA",
			"SDL_FRAMEWORK_COCOA",
			"SDL_FRAMEWORK_CARBON",
			"SDL_FILESYSTEM_COCOA",
		}

	filter "configurations:Debug"
		optimize "On"
		defines "NDEBUG"

	filter "configurations:Release"
		symbols "Off"
		optimize "Full"
		flags "LinkTimeOptimization"
		defines "NDEBUG"

	filter{}

	distcopy(mytarget())
