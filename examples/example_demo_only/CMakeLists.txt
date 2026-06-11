cmake_minimum_required(VERSION 3.3.0)
include(../CMakeUtils.cmake)

set(PROJECT_NAME "example_demo_only")

add_source_group(src ./ SRCS)
add_source_group(horus ../../src HUI_SRCS)
add_source_group(horus ../../include HUI_SRCS)
add_source_group(backends ../../backends HUI_BACKENDS)

add_definitions(-DHUI_STATIC)
add_executable(${PROJECT_NAME} ${SRCS} ${HUI_SRCS} ${HUI_BACKENDS} ${NFD_SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC
	../../include
	../../src
	../../backends
	../libs/sdl/include
	../libs/jsoncpp/include
	../libs/nativefiledialog/src/include
	../libs/stb
	../libs/utfcpp/source
	../libs
	../libs/freetype/include
	../libs/vulkan/Include)

link_libs(${PROJECT_NAME})

target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/bin)
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "../../bin")
