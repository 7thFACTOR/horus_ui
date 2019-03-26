project "without_docking"
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

	using { "horus", "sdl2" }
	distcopy(mytarget())