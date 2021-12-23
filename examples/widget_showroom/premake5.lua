project "widget_showroom"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++11"

  vpaths { ["backends"] = "../../backends/" }
  vpaths { ["horus"] = "../../src/" }
  vpaths { ["horus"] = "../../include/" }

	warnings "off"
  files {
    "../../src/*.*",
    "../../include/*.*",
    "../../backends/sdl2*.*",
    "../../backends/opengl*.*",
    "../../backends/binpack*.*",
    "../../backends/freetype*.*",
    "../../backends/stb_image*.*",
    "../../backends/stdio*.*",
    "../../backends/json*.*",
    "../../backends/utfcpp*.*",
    "../../backends/nativefiledialogs*.*",
    "*.cpp"
  }

	includedirs {
		".",
		"../../include",
		"../../backends",
    "../libs",
    "../libs/utfcpp/source"
	}

	defines "_CONSOLE"

	using { "sdl", "glew", "binpack", "freetype", "jsoncpp", "nativefiledialog" }
	distcopy(mytarget())