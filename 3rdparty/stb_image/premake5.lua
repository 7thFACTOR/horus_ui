project "stb_image"
	kind "StaticLib"
	language "C++"

	files "*.c"
	shared.includedirs "."	

	filter "system:windows"
		warnings "off"
		disablewarnings { 4224 }
		defines {
			"WIN32",
			"WIN32_LEAN_AND_MEAN",
			"VC_EXTRALEAN",
			"_CRT_SECURE_NO_WARNINGS"
		}

	filter "system:linux or macosx"
		buildoptions "-fPIC"
		defines {
			"WIN32_LEAN_AND_MEAN",
			"VC_EXTRALEAN",
			"_CRT_SECURE_NO_WARNINGS",
		}

	filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"
		defines "NDEBUG"

	filter{}
