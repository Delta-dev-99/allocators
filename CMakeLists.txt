cmake_minimum_required(VERSION 3.22.0)
project(allocators VERSION 0.1.0)

include(CTest)
enable_testing()

add_subdirectory(allocators)
add_subdirectory(tests)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
