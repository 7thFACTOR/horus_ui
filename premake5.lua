workspace "horus"

if _ACTION ~= "gmake" and _ACTION ~= "xcode" then
  function os.winSdkVersion()
    local reg_arch = iif( os.is64bit(), "\\Wow6432Node\\", "\\" )
    local sdk_version = os.getWindowsRegistry( "HKLM:SOFTWARE" .. reg_arch .."Microsoft\\Microsoft SDKs\\Windows\\v10.0\\ProductVersion" )
    if sdk_version ~= nil then return sdk_version end
  end

  filter {"system:windows", "action:vs*"}
    systemversion(os.winSdkVersion() .. ".0")
end

filter {}
-- Location of the solutions
if _ACTION == "vs2015" then
	location "./build_vs2015"
	-- We're on Windows, this will be used in code for ifdef
	defines {"_WINDOWS"}
	system ("windows")
end

-- Location of the solutions
if _ACTION == "vs2017" then
	location "./build_vs2017"
	-- We're on Windows, this will be used in code for ifdef
	defines {"_WINDOWS", "_WIN32"}
	system ("windows")
end

if _ACTION == "gmake" and _ARGS[1] == "macos" then
	location "./build_gmake"
	-- We're on Linux, this will be used in code for ifdef
	defines {"_APPLE"}
	system ("macosx")
else
	if _ACTION == "gmake" then
	location "./build_gmake"
	-- We're on Linux, this will be used in code for ifdef
	defines {"_LINUX"}
	system ("linux")
	end
end

if _ACTION == "xcode" then
	location "./build_xcode"
	-- We're on OSX, this will be used in code for ifdef
	defines {"_APPLE"}
	system ("macosx")
end

postbuildcommands { "{MKDIR} bin" }
postbuildcommands { "{COPY} \"%{cfg.linktarget.abspath}\" bin" }

language "C++"

-- We only support 64bit architectures
architecture "x64"

configurations { "Debug", "Release" }

-- Keep the script root to be used in paths
scriptRoot = _WORKING_DIR

-- With no filter, we set some global settings
filter {}
debugdir "bin"
targetdir "bin"
libdirs { "%{prj.location}\\bin" }

-- We support Unicode
characterset ("Unicode")

flags { "NoMinimalRebuild", "MultiProcessorCompile", "NoPCH" }
cppdialect "C++11"
filter { "system:windows" }
	-- Disable some warnings
	disablewarnings { 4251, 4006, 4221, 4204 }
filter {}

filter { "system:linux or macosx or ios" }
	buildoptions {"-pthread"}
	buildoptions {"-fpermissive"}
filter {}

---
--- Prepare the configurations
---

filter { "configurations:Debug" }
	symbols "On"
	optimize "Debug"
	defines {"_DEBUG"}
	warnings "Off"
	
filter { "configurations:Development" }
	symbols "On"
	optimize "Speed"
	defines {"_DEVELOPMENT"}
	defines {"NDEBUG"}
	warnings "Off"

filter { "configurations:Release" }
	optimize "Full"
	defines {"_SHIPPING"}
	defines {"_RELEASE"}
	defines {"NDEBUG"}
	warnings "Off"

---
--- Global defines
---
filter {}

-- used for new/delete override, with logging of new/delete operations
filter { "configurations:Debug or Development" }
filter {}

---
--- Utility functions
---
	
function add_sources_from(path)
	files { path.."**.h" }
	files { path.."**.hpp" }
	files { path.."**.cpp" }
	files { path.."**.cxx" }
	files { path.."**.c" }
	files { path.."**.inl" }
end

function add_res_from(path)
	files { path.."**/resource.h" }
	files { path.."**.res" }
	files { path.."**.rc" }
end

---
--- Linking with our libraries and 3rdparty
---

function link_win32()
	filter { "system:windows" }
		links { "shlwapi", "winmm",  "Ws2_32", "Wininet", "dbghelp" }
    links { "user32", "gdi32", "winmm", "imm32", "ole32", "oleaut32", "version", "uuid"}
	filter {}
end

function link_opengl()
	filter {}
	defines {"USE_OPENGL"}

	filter { "system:windows" }
		links { "opengl32" }

	filter { "system:linux" }
		links { "GL", "GLU", "X11" }
	filter {}
end

function link_freetype()
	filter {}
	includedirs {scriptRoot.."/3rdparty/freetype/include"}

	filter { "configurations:Debug"}
		links { "freetype_d" }
		
	filter { "configurations:Release"}
		links { "freetype" }
end

function link_glew()
	filter {}
	defines {"GLEW_STATIC"}
	includedirs {scriptRoot.."/3rdparty/glew/include"}

	filter { "configurations:Debug" }
		links { "glew_d" }
	filter {}

	filter { "configurations:Release" }
		links { "glew" }
	filter {}
end

function link_binpack()
	filter {}
	includedirs {scriptRoot.."/3rdparty/binpack"}

	filter { "configurations:Debug" }
		links { "binpack_d" }
	filter {}

	filter { "configurations:Release" }
		links { "binpack" }
	filter {}
end

function link_stb_image()
	filter {}
	includedirs {scriptRoot.."/3rdparty/stb_image"}

	filter { "configurations:Debug" }
		links { "stb_image_d" }
	filter {}

	filter { "configurations:Release" }
		links { "stb_image" }
	filter {}
end

function link_jsoncpp()
	filter {}
	includedirs {scriptRoot.."/3rdparty/jsoncpp/include"}

	filter { "configurations:Debug" }
		links { "jsoncpp_d" }
	filter {}

	filter { "configurations:Release" }
		links { "jsoncpp" }
	filter {}
end

function link_nfd()
	filter {}
	includedirs {scriptRoot.."/3rdparty/nativefiledialog/src/include"}

	filter { "configurations:Debug" }
		links { "nativefiledialog_d" }
	filter {}

	filter { "configurations:Release" }
		links { "nativefiledialog" }
	filter {}
end

function link_sdl2()
	filter {}
	includedirs {scriptRoot.."/3rdparty/sdl2/include"}

	filter { "configurations:Debug" }
		links { "sdl2" }
	filter {}

	filter { "configurations:Release" }
		links { "sdl2" }
	filter {}
end

function link_sfml()
	filter {"system:windows"}
		includedirs {scriptRoot.."/3rdparty/sfml/win64/include"}
		libdirs {scriptRoot.."/3rdparty/sfml/win64/lib"}
		filter { "system:windows", "configurations:Debug" }
			links { "freetype", "sfml-window-d", "sfml-system-d", "sfml-graphics-d", "sfml-audio-d" }
		filter { "system:windows", "configurations:Release" }
			links { "freetype", "sfml-window", "sfml-system", "sfml-graphics", "sfml-audio" }
	filter {}

	filter {"system:linux"}
		includedirs {scriptRoot.."/3rdparty/sfml/linux/include"}
		libdirs {scriptRoot.."/3rdparty/sfml/linux/lib"}
		filter { "system:linux", "configurations:Debug" }
			links { "freetype", "sfml-window-d", "sfml-system-d", "sfml-graphics-d", "sfml-audio-d" }
		filter { "system:linux", "configurations:Release" }
			links { "freetype", "sfml-window", "sfml-system", "sfml-graphics", "sfml-audio" }
	filter {}

	filter {"system:macosx"}
		includedirs {scriptRoot.."/3rdparty/sfml/macos/include"}
		libdirs {scriptRoot.."/3rdparty/sfml/macos/lib"}
		filter { "system:macosx", "configurations:Debug" }
			links { "sfml-window", "sfml-system", "sfml-graphics", "sfml-audio" }
		filter { "system:macosx", "configurations:Release" }
			links { "sfml-window", "sfml-system", "sfml-graphics", "sfml-audio" }
	filter {}
end

---
--- Own libs
---

function link_horus_static()
	filter {}
	includedirs {scriptRoot.."/include"}

	filter { "configurations:Debug" }
		links { "horus_sd" }
	filter {}

	filter { "configurations:Release" }
		links { "horus_s" }
	filter {}
end

function link_horus()
	filter {}
	includedirs {scriptRoot.."/include"}

	filter { "configurations:Debug" }
		links { "horus_d" }
	filter {}

	filter { "configurations:Release" }
		links { "horus" }
	filter {}
end

---
--- Include the subprojects
---

-- TODO: we need to generate solutions for CMake for 3rdparty that uses it
include "3rdparty"
include "src"
include "examples"
