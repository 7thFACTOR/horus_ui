project "jsoncpp"
	kind "StaticLib"
	language "C++"
	includedirs { 'include' }
	files {
		"src/lib_json/*.cpp",
	}
	defines
	{
		"WIN32",
		"WIN32_LEAN_AND_MEAN",
		"VC_EXTRALEAN",
		"_CRT_SECURE_NO_WARNINGS",
	}

  filter {"system:linux"}
    buildoptions {"-fPIC"}
  filter {}

	configuration "Debug"
		defines 
		{
		}
		symbols "On"
		targetname "jsoncpp_d"
	configuration "Release"
		defines
		{
			"NDEBUG"
		}
		optimize "On"
		targetname "jsoncpp"
