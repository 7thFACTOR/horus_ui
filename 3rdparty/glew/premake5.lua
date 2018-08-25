project "glew"
	kind "StaticLib"
	language "C++"
	includedirs { 'include' }
	files {
		"src/glew.c",
	}
	defines
	{
		"WIN32",
		"WIN32_LEAN_AND_MEAN",
		"VC_EXTRALEAN",
		"_CRT_SECURE_NO_WARNINGS",
		"GLEW_STATIC",
	}
  filter {"system:linux"}
    buildoptions {"-fPIC"}
  filter {}
	configuration "Debug"
		defines 
		{
		}
		symbols "On"
		targetname "glew_d"
	configuration "Release"
		defines
		{
			"NDEBUG"
		}
		optimize "On"
		targetname "glew"
