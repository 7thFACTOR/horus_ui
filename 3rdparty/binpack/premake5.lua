project "binpack"
	kind "StaticLib"
	language "C++"
	includedirs { 'include' }
	files {
		"*.cpp",
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
        defines {}
        symbols "On"
        targetname "binpack_d"
      
	configuration "Release"
        defines
        {
            "NDEBUG"
        }
        optimize "On"
        targetname "binpack"
