
add_executable(test1 test1.cpp)
set_target_properties(test1 PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(test1 allocators)

add_executable(test2 test2.cpp)
set_target_properties(test2 PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(test2 allocators)

add_executable(test3 test3.cpp)
set_target_properties(test3 PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(test3 allocators)
