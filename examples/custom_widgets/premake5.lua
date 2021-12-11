project "custom_widgets"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++11"

  vpaths { ["backends"] = "../../backends/" }
  vpaths { ["horus"] = "../../src/" }
  vpaths { ["horus"] = "../../include/" }

	warnings "off"
	files {
		"../../backends/sdl2*.h",
		"../../backends/sdl2*.cpp",
		"../../backends/opengl*.*",
    "../../src/*.*",
    "../../include/*.*",
		"*.cpp"
	}

	includedirs {
		".",
		"../../include",
		"../../backends"
	}
	
	defines "_CONSOLE"

	filter "system:linux"
		linkgroups 'On'

	filter{}

	using { "sdl", "glew" }
	distcopy(mytarget())
