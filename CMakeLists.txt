cmake_minimum_required(VERSION 2.8)
project(timeline2gif)

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
message(FATAL_ERROR "CMake generation is not allowed within the source directory!
Remove the CMakeCache.txt file and try again from another folder, e.g.:

   rm CMakeCache.txt
   mkdir build
   cd build
   cmake ..
")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${timeline2gif_SOURCE_DIR}/cmake)

set(T2G_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/timeline2gif/")
add_definitions(-DT2G_DATA_DIR="${T2G_DATA_DIR}")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(GD REQUIRED)

add_subdirectory(fonts)
add_subdirectory(src)

