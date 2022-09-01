cmake_minimum_required(VERSION 3.22.0)
project(allocators VERSION 0.3.3)

include(cmake/CompilerWarnings.cmake)

add_subdirectory(allocators)
if (PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
    add_subdirectory(tests)
endif ()
