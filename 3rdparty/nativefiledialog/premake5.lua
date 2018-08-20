project "nativefiledialog"
	kind "StaticLib"
	language "C++"
	includedirs { 'src/include' }

	filter {"system:windows"}
		files {
			"src/nfd_common.c",
			"src/nfd_win.cpp"
		}
		defines
		{
			"WIN32",
			--"WIN32_LEAN_AND_MEAN",
			--"VC_EXTRALEAN",
			--"_CRT_SECURE_NO_WARNINGS",
		}
	filter {}
	
	filter {"system:linux"}
		files
		{
			"src/nfd_common.c",
			"src/nfd_gtk.c"
		}
		defines
		{
		}
		buildoptions {"-fPIC"}
		buildoptions {"`pkg-config --cflags gtk+-3.0`"}
		linkoptions {"`pkg-config --libs gtk+-3.0`"}
	filter {}
	
	filter {"system:macosx"}
		files
		{
			"src/nfd_common.c",
			"src/nfd_cocoa.m"
		}
		defines
		{
		}
		buildoptions {"-fPIC -fno-exceptions"}
	filter {}

	configuration "Debug"
		defines 
		{
		}
		symbols "On"
		targetname "nativefiledialog_d"

	configuration "Release"
		defines
		{
			"NDEBUG"
		}

		optimize "On"
		targetname "nativefiledialog"
