cmake_minimum_required(VERSION 3.30.0)
project(allocators VERSION 0.4.0)

include(cmake/CompilerWarnings.cmake)

add_subdirectory(allocators)
if (PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
    add_subdirectory(tests)
endif ()
