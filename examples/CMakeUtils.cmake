set(CODE_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(PLATFORM "")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_CL_64)
	set(PLATFORM_POSTFIX "x64")
else(CMAKE_CL_64)
	set(PLATFORM_POSTFIX "")
endif(CMAKE_CL_64)

if(UNIX OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC)
	add_definitions(-DPLATFORM_LINUX)
	set(LINUX 1)
endif(UNIX OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC)

if(APPLE)
	add_definitions(-DPLATFORM_APPLE)
endif(APPLE)

set(BIN_FOLDER bin)
set(PROJECTS_DIR $ENV{PROJECTS_FOLDER})
set(FULL_PROJECTS_PATH $ENV{FULL_PROJECTS_PATH})
option(BUILD_SHARED_LIBS "Build a shared library form of zlib" OFF)

if(WIN32)
	add_definitions(-DPLATFORM_WIN)
	add_definitions(/W0)
	add_definitions(/MP8)
endif(WIN32)

if(LINUX)
	add_definitions(-std=c++11)
	add_definitions(-w)
endif(LINUX)

link_directories(${FULL_PROJECTS_PATH}/lib)

macro(add_source_group FILTER_NAME SOURCE_PATH TARGET_LIST)
   file(TO_NATIVE_PATH ${FILTER_NAME} NEW_FILTER_NAME)
   if (WIN32 OR LINUX)
      file(GLOB TEMP_SRC
         "${SOURCE_PATH}/*.h"
         "${SOURCE_PATH}/*.inl"
         "${SOURCE_PATH}/*.cpp"
         "${SOURCE_PATH}/*.c"
         "${SOURCE_PATH}/*.cxx"
         "${SOURCE_PATH}/*.cc"
      )
   endif(WIN32 OR LINUX)
   if (APPLE)
      file(GLOB TEMP_SRC
         "${SOURCE_PATH}/*.h"
         "${SOURCE_PATH}/*.cpp"
         "${SOURCE_PATH}/*.mm"
         "${SOURCE_PATH}/*.c"
         "${SOURCE_PATH}/*.cxx"
         "${SOURCE_PATH}/*.cc"
      )
   endif(APPLE)
   source_group("${NEW_FILTER_NAME}" FILES ${TEMP_SRC})
   list(APPEND ${TARGET_LIST} "${TEMP_SRC}")
endmacro(add_source_group)

macro(link_libs TARGET_LIST)
	if(WINDOWS)
		target_link_libraries(${TARGET_LIST} PUBLIC "shlwapi"  "Ws2_32" "Wininet" "dbghelp"
		  "user32" "gdi32" "ole32" "oleaut32"  "uuid" "opengl32" "winmm" "setupapi" "version" "imm32")
	endif(WINDOWS)

	if(LINUX)
		target_link_libraries(${TARGET_LIST} PUBLIC "X11 `pkg-config --libs gtk+-3.0`"
		  "Xi" "dl" "pthread" "Xext" "GL" "GLU")
	endif(LINUX)

	if(APPLE)
		target_link_libraries(${TARGET_LIST} PUBLIC "dl" "ForceFeedback.framework" "CoreVideo.framework" "Cocoa.framework"
		  "IOKit.framework" "Carbon.framework" "CoreAudio.framework" "AudioToolbox.framework" "OpenGL.framework")
	endif(APPLE)
	
	if(LINUX)
		find_package(GTK REQUIRED)
		find_package(PkgConfig)
		pkg_check_modules(GTK "gtk+-3.0")
		target_link_libraries(${TARGET_LIST} PUBLIC ${GTK_LIBRARIES})
		add_definitions(${GTK_CFLAGS} ${GTK_CFLAGS_OTHER} -DMAKE_LIB)

		#target_compile_options(${TARGET_LIST} PRIVATE -Werror=return-type -std=c++17 -lstdc++fs)

		if(CMAKE_BUILD_TYPE MATCHES Debug)
			target_link_libraries(${TARGET_LIST} PUBLIC libSDL2-2.0d.so libSDL2_mixer-2.0d.so libGLEWd.a libjsoncpp.a libGL.so libspdlogd.so)
		else(CMAKE_BUILD_TYPE MATCHES Debug)
			target_link_libraries(${TARGET_LIST} PUBLIC libSDL2-2.0.so libSDL2_mixer-2.0.so libGLEW.so libjsoncpp.so libGL.so libspdlog.so)
		endif(CMAKE_BUILD_TYPE MATCHES Debug)
	endif(LINUX)

	if (WIN32)
		target_link_libraries(${TARGET_LIST} PUBLIC debug freetyped optimized freetype)
		target_link_libraries(${TARGET_LIST} PUBLIC debug SDL3 optimized SDL3)
		#target_link_libraries(${TARGET_LIST} PUBLIC debug SDL3_mixerd optimized SDL3_mixer)
		#target_link_libraries(${TARGET_LIST} PUBLIC debug libGLEW32d optimized libGLEW32)
		#target_link_libraries(${TARGET_LIST} PUBLIC debug glfw3 optimized glfw3)
		target_link_libraries(${TARGET_LIST} PUBLIC debug jsoncpp_static optimized jsoncpp_static)
		#target_link_libraries(${TARGET_LIST} PUBLIC debug spdlogd optimized spdlog)
	endif(WIN32)
	
endmacro(link_libs)
