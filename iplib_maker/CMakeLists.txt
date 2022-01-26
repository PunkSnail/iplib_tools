cmake_minimum_required(VERSION 2.6)
project(iplib_maker)

set (CMAKE_C_COMPILER ${CMAKE_CXX_COMPILER}) 

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -g")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion")

include_directories(${PROJECT_SOURCE_DIR}/inc)

aux_source_directory(${PROJECT_SOURCE_DIR}/src IPLIB_MAKER_SOURCES)

add_executable(iplib_maker ${IPLIB_MAKER_SOURCES})

#target_link_libraries(iplib_maker )
