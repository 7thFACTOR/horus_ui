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

add_definitions(-DSDL_MAIN_AVAILABLE)

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

#add_subdirectory(custom_widgets)
#add_subdirectory(widget_showroom)
add_subdirectory(without_docking)
#add_subdirectory(docking_test)