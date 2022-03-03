cmake_minimum_required(VERSION 3.22.0)
project(allocators VERSION 0.1.0)

include(cmake/CompilerWarnings.cmake)

add_subdirectory(allocators)
add_subdirectory(tests)
