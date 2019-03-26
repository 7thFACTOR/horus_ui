project "binpack"
	kind "StaticLib"
	language "C++"

	files "*.cpp"
	shared.includedirs "."

	defines {
        "WIN32",
        "WIN32_LEAN_AND_MEAN",
        "VC_EXTRALEAN",
        "_CRT_SECURE_NO_WARNINGS",
    }

	filter "system:windows"
		disablewarnings { 4267 }

	filter "system:linux"
		buildoptions "-fPIC"

	filter "configurations:Debug"
        symbols "On"

	filter "configurations:Release"
        defines "NDEBUG"
        optimize "On"

	filter{}