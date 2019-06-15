project "widget_showroom"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++11"

	warnings "off"
	files {
		"../common/sdl2*.*",
		"../common/opengl*.*",
		"*.cpp"
	}

	includedirs {
		".",
		"../../include",
		"../common"
	}

	defines "_CONSOLE"

	using { "horus", "sdl" }
	distcopy(mytarget())

project "widget_showroom_static"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++11"

	warnings "off"
	files {
		"../common/sdl2*.h",
		"../common/sdl2*.cpp",
		"../common/opengl*.h",
		"../common/opengl*.cpp",
		"*.cpp"
	}

	includedirs {
		".",
		"../../include",
		"../common"
	}

	defines "_CONSOLE"

	using { "horus_static", "sdl" }
	dumpModules()
  distcopy(mytarget())
