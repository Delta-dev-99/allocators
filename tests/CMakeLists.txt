
add_executable(test2 test2.cpp)
set_target_properties(test2 PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(test2 allocators)

add_executable(test3 test3.cpp)
set_target_properties(test3 PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(test3 allocators)

add_executable(stats stats.cpp)
set_target_properties(stats PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(stats allocators)
