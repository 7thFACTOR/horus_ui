project "horus"
	kind "SharedLib"
	defines {"HORUS_EXPORTS"}
	includedirs {scriptRoot, scriptRoot.."/include"}
	add_sources_from("./")
	add_sources_from("../include/")
	vpaths { ["*"] = { scriptRoot.."/src/**.*", scriptRoot.."/include/**.*" } }

	link_binpack()
	link_win32()
	link_opengl()
	link_glew()
	link_freetype()
	link_jsoncpp()
	link_stb_image()
	link_nfd()
	link_sdl2()

	filter {"system:macosx"}
		links {"OpenGL.framework", "ForceFeedback.framework", "CoreVideo.framework", "Cocoa.framework", "IOKit.framework", "Carbon.framework", "CoreAudio.framework", "AudioToolbox.framework", "dl"}
	filter {}

	configuration "Debug"
        	defines {}
	        symbols "On"
	        targetname "horus_d"

	configuration "Release"
	        defines
	        {
	            "NDEBUG"
	        }
	        optimize "On"
	        targetname "horus"

project "horus_s"
	kind "StaticLib"
	defines {"HORUS_STATIC"}
	includedirs {scriptRoot, scriptRoot.."/include"}
	add_sources_from("./")
	vpaths { ["*"] = scriptRoot.."/src/**.*" }

	link_binpack()
	link_win32()
	link_opengl()
	link_glew()
	link_freetype()
	link_jsoncpp()
	link_stb_image()
	link_nfd()
	link_sdl2()

	filter {"system:macosx"}
		links {"OpenGL.framework", "ForceFeedback.framework", "CoreVideo.framework", "Cocoa.framework", "IOKit.framework", "Carbon.framework", "CoreAudio.framework", "AudioToolbox.framework", "dl"}
	filter {}

	configuration "Debug"
        	defines {}
	        symbols "On"
	        targetname "horus_sd"

	configuration "Release"
	        defines
	        {
	            "NDEBUG"
	        }
	        optimize "On"
	        targetname "horus_s"
