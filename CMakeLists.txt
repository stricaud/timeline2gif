cmake_minimum_required(VERSION 3.10)
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
find_package(FLEX  REQUIRED)
find_package(GD    REQUIRED)

if(NOT FLEX_INCLUDE_DIRS)
    find_path(FLEX_INCLUDE_DIRS FlexLexer.h
        HINTS /opt/homebrew/opt/flex/include   # macOS Homebrew custom flex
              /opt/homebrew/include
              /usr/local/include
              /usr/include                      # Linux system flex
              "$ENV{MSYSTEM_PREFIX}/include")   # MSYS2: /mingw64, /ucrt64, …
endif()

# If the search above still failed, clear the -NOTFOUND sentinel so it is
# never passed as a literal include path.  FlexLexer.h will still be found
# via the compiler's default search path on most Unix systems.
if(NOT FLEX_INCLUDE_DIRS OR FLEX_INCLUDE_DIRS MATCHES "NOTFOUND")
    unset(FLEX_INCLUDE_DIRS CACHE)
    set(FLEX_INCLUDE_DIRS "")
endif()

add_subdirectory(src)

option(BUILD_UI "Build the wxWidgets GUI (requires wxWidgets)" OFF)
if(BUILD_UI)
    add_subdirectory(ui)
endif()
