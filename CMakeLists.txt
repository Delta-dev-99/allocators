cmake_minimum_required(VERSION 3.21.0)
project(allocators VERSION 0.4.0)

include(cmake/CompilerWarnings.cmake)

add_subdirectory(allocators)
if (PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
    enable_testing()
    add_subdirectory(tests)
endif ()
