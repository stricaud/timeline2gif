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

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(GD REQUIRED)

add_subdirectory(src)

