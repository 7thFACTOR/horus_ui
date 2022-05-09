# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required(VERSION 2.8.11)
include(../CMakeUtils.cmake)

add_source_group(src ./ SRCS)
add_source_group(backends ../../backends BACKENDS)
add_source_group(binpack ../libs/binpack BINPACK)


add_executable(widget_showroom ${SRCS} ${BACKENDS} ${BINPACK})
target_include_directories(widget_showroom PUBLIC ../../include ../../src ../../backends ../libs/sdl/include ../libs/glew/include
	../libs/jsoncpp/include ../libs/nativefiledialog/src/include ../libs/stb ../libs/utfcpp/source ../libs ../libs/binpack ../libs/freetype/include)
target_link_directories(widget_showroom PUBLIC ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(widget_showroom libSDL2-2.0 libfreetype libGLEW libjsoncpp) 

