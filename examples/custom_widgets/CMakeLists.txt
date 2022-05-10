# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required(VERSION 2.8.11)
include(../CMakeUtils.cmake)

add_source_group(src ./ SRCS)
add_source_group(horus ../../src HORUS_SRCS)
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
add_executable(custom_widgets ${SRCS} ${HORUS_SRCS} ${BACKENDS} ${BINPACK} ${NFD_SRCS})

target_include_directories(custom_widgets PUBLIC
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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++0x -O3 -fPIC" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3 -fPIC" )

find_package(GTK REQUIRED)
find_package(PkgConfig)
pkg_check_modules(GTK "gtk+-3.0")
target_link_libraries(custom_widgets ${GTK_LIBRARIES})
add_definitions(${GTK_CFLAGS} ${GTK_CFLAGS_OTHER})

target_link_directories(custom_widgets PUBLIC ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(custom_widgets libSDL2-2.0.so libfreetype.so libGLEW.so libjsoncpp.so libGL.so) 

