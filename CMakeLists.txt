cmake_minimum_required(VERSION 3.16)
project(BUAA_SysY_Compiler_2025 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Optional: assert FunctionType uniquify in debug builds via compile definition.
# (Call sites remain getFunctionTyImpl; pointer equality is the contract.)
