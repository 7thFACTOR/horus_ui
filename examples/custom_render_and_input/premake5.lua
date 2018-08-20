project "custom_render_and_input"
	kind "ConsoleApp"
	defines {"_CONSOLE", "HORUS_IMPORTS"}
	includedirs {scriptRoot, scriptRoot.."/include"}
	add_sources_from("./")
	link_binpack()
	link_win32()
	link_opengl()
	link_glew()
	link_freetype()
	link_jsoncpp()
	link_stb_image()
	link_nfd()
	link_sfml()
	link_horus()
	link_sdl2()
	filter {"system:linux"}
	linkgroups 'On'
		buildoptions {"`pkg-config --cflags gtk+-3.0` -fPIC"}
		links {"X11 `pkg-config --libs gtk+-3.0`"}
		links {"Xi", "dl", "pthread", "Xrandr", "udev" }
	filter {}
		filter {"system:macosx"}
		links {"OpenGL.framework", "ForceFeedback.framework", "CoreVideo.framework", "Cocoa.framework", "IOKit.framework", "Carbon.framework", "CoreAudio.framework", "AudioToolbox.framework", "dl"}
	filter {}
	configuration "Debug"
		defines {}
		symbols "On"
		targetname "custom_render_and_input_d"
	configuration "Release"
		defines
		{
			"NDEBUG"
		}
		optimize "On"
		targetname "custom_render_and_input"
