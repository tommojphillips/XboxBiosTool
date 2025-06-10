cmake_minimum_required(VERSION 3.30)
project(xbox_bios_tool_tests)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Og")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Og")

enable_testing()
find_package(Boost 1.70.0 REQUIRED COMPONENTS unit_test_framework)

# Tests ----------------------------------------------

add_executable(
        compressor_tests
        compress/test_main.cpp
)

target_link_libraries(
        compressor_tests
        PRIVATE
        compressor_lib
        Boost::unit_test_framework
)

add_test(
        NAME
        compressor_tests
        COMMAND
        compressor_tests
)
