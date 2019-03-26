project "glew"
	kind "StaticLib"
	language "C++"

	files "src/glew.c"
	shared.includedirs "include"
	shared.defines "GLEW_STATIC"

	if os.target() == "windows" then
		shared.defines "WIN32"
	elseif os.target() == "macosx" then
		shared.defines "__APPLE__"
	end
	
 	filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"
		defines "NDEBUG"

	filter "system:windows"
		defines { "WIN32_LEAN_AND_MEAN", "VC_EXTRALEAN", "_CRT_SECURE_NO_WARNINGS" }

	filter "system:linux"
		buildoptions "-fPIC"

	filter{}

	using "opengl"
