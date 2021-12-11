project "nativefiledialog"
	kind "StaticLib"
	language "C++"

	shared.includedirs "nativefiledialog/src/include"

	if os.target() == "windows" then
		files {
			"nativefiledialog/src/nfd_common.c",
			"nativefiledialog/src/nfd_win.cpp"
		}
	elseif os.target() == "linux" then
		files {
			"nativefiledialog/src/nfd_common.c",
			"nativefiledialog/src/nfd_gtk.c"
		}
	elseif os.target() == "macosx" then
		files {
			"nativefiledialog/src/nfd_common.c",
			"nativefiledialog/src/nfd_cocoa.m"
		}
	end
--[[

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
]]--
	filter "system:windows"
		disablewarnings { 4996 }
		defines {
			"WIN32",
			--"WIN32_LEAN_AND_MEAN",
			--"VC_EXTRALEAN",
			--"_CRT_SECURE_NO_WARNINGS",
		}

	filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"
		defines "NDEBUG"

	filter "system:linux"
		buildoptions { "-fPIC", "`pkg-config --cflags gtk+-3.0`" }
		linkoptions "`pkg-config --libs gtk+-3.0`"

	filter "system:macosx"
		buildoptions "-fPIC -fno-exceptions"

	filter{}