cmake_minimum_required(VERSION 2.6)
project(iplib_reader)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_DEBUG -Wconversion -g -Wall")

include_directories(inc)

aux_source_directory(${PROJECT_SOURCE_DIR}/src IP_READER_SOURCES)

add_executable(iplib_reader ${IP_READER_SOURCES})

target_link_libraries(iplib_reader)

