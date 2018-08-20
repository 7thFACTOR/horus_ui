project "stb_image"
	kind "StaticLib"
	language "C++"
	includedirs { 'include' }
	files {"*.c"}
	filter {"system:windows"}
		defines
		{
			"WIN32",
			"WIN32_LEAN_AND_MEAN",
			"VC_EXTRALEAN",
			"_CRT_SECURE_NO_WARNINGS",
		}
	filter {}
	filter {"system:linux or macosx"}
		defines
		{
			"WIN32_LEAN_AND_MEAN",
			"VC_EXTRALEAN",
			"_CRT_SECURE_NO_WARNINGS",
		}
		buildoptions {"-fPIC"}
	filter {}
	
	configuration "Debug"
		defines 
		{
		}
		symbols "On"
		targetname "stb_image_d"
	configuration "Release"
		defines
		{
			"NDEBUG"
		}
		optimize "On"
		targetname "stb_image"