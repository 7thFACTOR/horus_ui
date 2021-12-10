project "custom_widgets"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++11"

	warnings "off"
	files {
		"../common/sdl2*.h",
		"../common/sdl2*.cpp",
		"../common/opengl*.*",
		"*.cpp"
	}

	includedirs {
		".",
		"../../include",
		"../common"
	}
	
	defines "_CONSOLE"

	filter "system:linux"
		linkgroups 'On'

	filter{}

	using { "horus", "sdl" }	
	distcopy(mytarget())
