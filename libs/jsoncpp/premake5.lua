project "jsoncpp"
	kind "StaticLib"
	language "C++"

	files "src/lib_json/*.cpp"
	shared.includedirs "include"

	defines {
		"WIN32",
		"WIN32_LEAN_AND_MEAN",
		"VC_EXTRALEAN",
		"_CRT_SECURE_NO_WARNINGS",
	}

	filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"
		defines "NDEBUG"

	filter "system:linux"
		buildoptions "-fPIC"

	filter{}