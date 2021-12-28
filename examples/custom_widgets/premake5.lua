project "custom_widgets"
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
    "../../backends/stb_rectpack*.*",
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

	filter "system:linux"
		linkgroups 'On'

	filter{}

	using { "sdl", "glew", "binpack", "freetype", "jsoncpp", "nativefiledialog" }
	distcopy(mytarget())
