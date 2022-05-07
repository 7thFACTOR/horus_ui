# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required(VERSION 2.8.11)
include(../CMakeUtils.cmake)

add_source_group(src ./ SRCS)

add_executable(widget_showroom ${SRCS})

target_link_libraries(widget_showroom libSDL2-2.0) 
