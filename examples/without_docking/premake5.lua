project "without_docking"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++11"

  vpaths { ["backends"] = "../../backends/" }
  vpaths { ["horus"] = "../../src/" }
  vpaths { ["horus"] = "../../include/" }

	warnings "off"
	files {
		"../../backends/sdl2*.*",
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

	using { "sdl", "glew" }
	distcopy(mytarget())