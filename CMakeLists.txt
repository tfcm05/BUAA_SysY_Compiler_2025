cmake_minimum_required(VERSION 3.16)
project(BUAA_SysY_Compiler_2025 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug: fast rebuild (-O0). Release: optimize the compiler binary (-O2).
# Usage:
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug|Release)" FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Speed up rebuilds when ccache is installed.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  message(STATUS "ccache: ${CCACHE_PROGRAM}")
else()
  message(STATUS "ccache: not found (optional)")
endif()

file(GLOB_RECURSE COMPILER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(Compiler
    Compiler.cpp
    ${COMPILER_SOURCES}
)

# Compiler.cpp uses "include/...", sources also use -Iinclude style paths.
target_include_directories(Compiler PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Parallel-friendly: cmake --build build -j$(nproc)
