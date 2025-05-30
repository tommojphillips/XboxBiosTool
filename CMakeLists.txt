cmake_minimum_required(VERSION 3.18)

project(XboxBiosTool)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

set(CMAKE_CXX_STANDARD 17)
include(CMakePushCheckState)
set(_CMAKE_PROCESSING_LANGUAGE "C")
include(CheckSymbolExists)
include (ExternalProject)
include(FindPkgConfig)

set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(
        xbox_bios_tool
        src/Bios.cpp
        src/Mcpx.c
        src/XbTool.cpp
        src/XcodeDecoder.cpp
        src/XcodeInterp.cpp
        src/cli_tbl.cpp
        src/file.c
        src/loadini.c
        src/lzx_decoder.c
        src/lzx_encoder.c
        src/mem_tracking.cpp
        src/nt_headers.c
        src/rc4.c
        src/rsa.c
        src/sha1.c
        src/str_util.c
        src/tea.c
        src/util.c
        inc/posix_shims.h
)

target_include_directories(
        xbox_bios_tool
        PRIVATE
        inc
)

target_compile_definitions(
        xbox_bios_tool
        PUBLIC
        -D __STDC_WANT_LIB_EXT1__=1
)