require("vstudio")
require('remake')

if HORUS_NO_WORKSPACE then
  project "horus"
else
  workspace "horus"
end

configurations { "Debug", "Development", "Release" }

-- global options
architecture "x64" -- We only support 64bit architectures
characterset "Unicode" -- We support Unicode
flags { "NoMinimalRebuild", "MultiProcessorCompile", "NoPCH" }

-- build config
if _ACTION == "vs2019" then
  system "windows"
  isWindows = true
elseif _ACTION == "gmake" then
  if _ARGS[1] == "macos" then
    system "macosx"
  else
    system "linux"
  end
  isLinux = true
elseif _ACTION == "xcode" then
  system "macosx"
  isLinux = true
else
  premake.error("Unknown/Unsupported build action: " .. _ACTION)
end

paths = Config('src', 'libs', ('build_' .. _ACTION), 'bin')

--- prepare naming convention for library files
filter "kind:SharedLib or StaticLib"
  pic "on"

filter { "kind:SharedLib or *App", "configurations:Debug or Development" }
  targetsuffix '_d'

filter { "kind:StaticLib", "configurations:Debug or Development" }
  targetsuffix '_sd'
  
--- prepare the configurations
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
  if isWindows then systemversion(os.winSdkVersion() .. ".0") end
  
filter { "system:linux or macosx or ios" }
  buildoptions { "-pthread", "-fpermissive" }

filter "system:windows"
  -- disable some warnings
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

group "libs"
  include "libs"

group "horus"
  include "src"

group "examples"
  includeall "examples"