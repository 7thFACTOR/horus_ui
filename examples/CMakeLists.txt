# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required(VERSION 3.0.0)
project(horus_examples)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}")
add_definitions(-DSDL_MAIN_HANDLED -DOpenGL_GL_PREFERENCE=GLVND)

if(WIN32)
	add_definitions(-DPLATFORM_WIN)
	add_definitions(/W0)
	add_definitions(/MP8)
	set(WINDOWS 1)
endif(WIN32)

if(WINDOWS)
	link_libraries("shlwapi"  "Ws2_32" "Wininet" "dbghelp"
      "user32" "gdi32" "ole32" "oleaut32"  "uuid" "opengl32" "winmm" "setupapi" "version" "imm32")
endif(WINDOWS)

if(LINUX)
	link_libraries("X11 `pkg-config --libs gtk+-3.0`"
      "Xi" "dl" "pthread" "Xext" "GL" "GLU")
endif(LINUX)

if(APPLE)
	link_libraries("dl" "ForceFeedback.framework" "CoreVideo.framework" "Cocoa.framework"
      "IOKit.framework" "Carbon.framework" "CoreAudio.framework" "AudioToolbox.framework" "OpenGL.framework")
endif(APPLE)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/../bin)

link_directories(${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/../bin)

add_subdirectory(libs)

if(LINUX)
find_package(GTK REQUIRED)
find_package(PkgConfig)
pkg_check_modules(GTK "gtk+-3.0")
link_libraries(${GTK_LIBRARIES})
add_definitions(${GTK_CFLAGS} ${GTK_CFLAGS_OTHER} -DMAKE_LIB)

#target_compile_options(shmup PRIVATE -Werror=return-type -std=c++17 -lstdc++fs)

if(CMAKE_BUILD_TYPE MATCHES Debug)
	link_libraries(libSDL2-2.0d.so libSDL2_mixer-2.0d.so libGLEWd.a libjsoncpp.a libGL.so libspdlogd.so)
else(CMAKE_BUILD_TYPE MATCHES Debug)
	link_libraries(libSDL2-2.0.so libSDL2_mixer-2.0.so libGLEW.so libjsoncpp.so libGL.so libspdlog.so)
endif(CMAKE_BUILD_TYPE MATCHES Debug)
endif(LINUX)

if (WIN32)
	link_libraries(debug freetyped optimized freetype)
	link_libraries(debug SDL2d optimized SDL2)
	#link_libraries(debug SDL2_mixerd optimized SDL2_mixer)
	link_libraries(debug libGLEW32d optimized libGLEW32)
	link_libraries(debug jsoncpp_static optimized jsoncpp_static)
	#link_libraries(debug spdlogd optimized spdlog)
endif(WIN32)

add_subdirectory(custom_widgets)
add_subdirectory(widget_showroom)
add_subdirectory(without_docking)
add_subdirectory(docking_test)