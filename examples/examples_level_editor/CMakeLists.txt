cmake_minimum_required(VERSION 3.5)
include(../CMakeUtils.cmake)

set(PROJECT_NAME "examples_level_editor")

add_source_group(src ./ SRCS)
add_source_group(horus ../../src HUI_SRCS)
add_source_group(horus ../../include HUI_SRCS)
add_source_group(backends ../../backends HUI_BACKENDS)

add_definitions(-DHUI_STATIC)

if(WIN32)
	get_filename_component(DPI_AWARENESS_MANIFEST
		${CMAKE_CURRENT_SOURCE_DIR}/../common/dpi_awareness.manifest ABSOLUTE)
	file(TO_CMAKE_PATH "${DPI_AWARENESS_MANIFEST}" DPI_AWARENESS_MANIFEST)
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/../common/dpi_awareness.rc.in
		${CMAKE_CURRENT_BINARY_DIR}/dpi_awareness.rc
		@ONLY)

	set(PLATFORM_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/dpi_awareness.rc)
	source_group("resources" FILES ${PLATFORM_RESOURCES})
endif(WIN32)

add_executable(${PROJECT_NAME} ${SRCS} ${HUI_SRCS} ${HUI_BACKENDS} ${NFD_SRCS} ${PLATFORM_RESOURCES})

if(MSVC)
	# use the embedded manifest from dpi_awareness.rc instead of the linker-generated one
	set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS " /MANIFEST:NO")
endif(MSVC)

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
