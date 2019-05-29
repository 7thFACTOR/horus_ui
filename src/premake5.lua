function horus_base()
	language "C++"
	cppdialect "C++11"

	files {
		"**.h",
		"**.hpp",
		"**.cpp",
		"**.cxx",
		"**.c",
		"**.inl",
		"../include/**.h",
		"../include/**.hpp",
		"../include/**.cpp",
		"../include/**.cxx",
		"../include/**.c",
		"../include/**.inl",
	}

	vpaths { ["*"] = { "src/**.*", "include/**.*" } }

	shared.includedirs {
		".",
		"..",
		"../include"
	}

	public.defines "HORUS_IMPORTS"
	defines { "HORUS_EXPORTS", "HORUS_TIMING_DEBUG" }

	warnings "off"
	using {	"os", "binpack", "stb", "jsoncpp", "nativefiledialog", "freetype", "glew" } 
end

project "horus"
	kind "SharedLib"
	horus_base()
	distcopy(mytarget())

project "horus_static"
	kind "StaticLib"
	horus_base()
	shared.defines "HORUS_STATIC"
