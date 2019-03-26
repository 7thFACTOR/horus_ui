require('remake')
workspace "horus"
	configurations { "Debug", "Development", "Release" }
	
	-- global options
	architecture "x64" -- We only support 64bit architectures
	characterset "Unicode" -- We support Unicode
	flags { "NoMinimalRebuild", "MultiProcessorCompile", "NoPCH" }

	-- Build config
	if _ACTION == "vs2015" or _ACTION == "vs2017" then
		system "windows"
	elseif _ACTION == "gmake" then
		if _ARGS[1] == "macos" then
			system "macosx"
		else
			system "linux"
		end
	elseif _ACTION == "xcode" then
		system "macosx"
	else
		premake.error("Unknown/Unsupported build action specifie: " .. _ACTION)
	end

	paths = Config('src', '3rdparty', ('build_' .. _ACTION), 'bin')

	--- Prepare naming convention of libs
	filter "kind:SharedLib or StaticLib"
		pic "on"

	filter { "kind:SharedLib or *App", "configurations:Debug or Development" }
		targetsuffix '_d'

	filter { "kind:StaticLib", "configurations:Debug or Development" }
		targetsuffix '_sd'
		
	--- Prepare the configurations
	filter "configurations:Debug"
		symbols "On"
		optimize "Debug"
		defines "_DEBUG"
		warnings "Off"

	filter "configurations:Development"
		symbols "On"
		optimize "Speed"
		defines { "_DEVELOPMENT", "NDEBUG" }
		warnings "Off"

	filter "configurations:Release"
		symbols "Off"
		optimize "Full"
		flags "LinkTimeOptimization"
		defines { "_SHIPPING", "_RELEASE", "NDEBUG" }
		warnings "Off"

	filter { "system:windows", "action:vs*" }
		systemversion(os.winSdkVersion() .. ".0")
		
	filter { "system:linux or macosx or ios" }
		buildoptions { "-pthread", "-fpermissive" }

	filter "system:windows"
		-- Disable some warnings		
		disablewarnings { 4251, 4006, 4221, 4204 }
		defines "_WINDOWS"

	filter "system:macosx or ios"
		defines "_APPLE"

	filter "system:linux"
		defines "_LINUX"

	filter{}


	library("os", function()
		if os.target() == "windows" then
			public.links {
				"shlwapi",  "Ws2_32", "Wininet", "dbghelp",
				"user32", "gdi32", "ole32", "oleaut32",  "uuid"
			}
		elseif os.target() == "linux" then
			public.links {
				"X11 `pkg-config --libs gtk+-3.0`",
				"Xi", "dl", "pthread", "Xext"
			}
		elseif os.target() == "macosx" then
			public.links { 
				"dl", "ForceFeedback.framework", "CoreVideo.framework", "Cocoa.framework",
				"IOKit.framework", "Carbon.framework", "CoreAudio.framework", "AudioToolbox.framework",
			}			
		end
	end)

	library("opengl", function()
		public.defines "USE_OPENGL"

		if os.target() == "windows" then
			public.links "opengl32"
		elseif os.target() == "linux" then
			public.links { "GL", "GLU", "X11" }
		elseif os.target() == "macosx" then
			public.links "OpenGL.framework"
		end
	end)
	
	group "3rdparty"
		include "3rdparty"

	group "horus-ui"
		include "src"

	group "examples"
		includeall "examples"
