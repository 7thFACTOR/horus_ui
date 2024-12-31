cmake_minimum_required(VERSION 3.3.0)
include(../CMakeUtils.cmake)

add_source_group(src ./ SRCS)
add_source_group(horus ../../src HORUS_SRCS)
add_source_group(horus ../../include HORUS_SRCS)
add_source_group(backends ../../backends BACKENDS)
add_source_group(binpack ../libs/binpack BINPACK)

if(LINUX)
list(APPEND NFD_SRCS
	../libs/nativefiledialog/src/nfd_common.c
	../libs/nativefiledialog/src/nfd_gtk.c
	)
endif(LINUX)

if(WINDOWS)
list(APPEND NFD_SRCS
	../libs/nativefiledialog/src/nfd_common.c
	../libs/nativefiledialog/src/nfd_win.cpp
	)
endif(WINDOWS)


add_definitions(-DHORUS_STATIC)
add_executable(without_docking ${SRCS} ${HORUS_SRCS} ${BACKENDS} ${BINPACK} ${NFD_SRCS})

target_include_directories(without_docking PUBLIC
	../../include
	../../src
	../../backends
	../libs/sdl/include
	../libs/glew/include
	../libs/jsoncpp/include
	../libs/nativefiledialog/src/include
	../libs/stb
	../libs/utfcpp/source
	../libs
	../libs/binpack
	../libs/freetype/include)

link_libs(without_docking)

target_link_directories(without_docking PUBLIC ${CMAKE_BINARY_DIR}/bin)
set_target_properties(without_docking PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "../../bin")
