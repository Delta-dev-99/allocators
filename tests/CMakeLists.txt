
add_executable(test2 test2.cpp)
# set_target_properties(test2 PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(test2 dd99::allocators)
add_test(NAME test2 COMMAND test2)

add_executable(test3 test3.cpp)
# set_target_properties(test3 PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(test3 dd99::allocators)
add_test(NAME test3 COMMAND test3)

# stats is a manual benchmarking tool (prints timing figures, no pass/fail
# semantics), not registered as an automated test -- it's still built here so
# compile regressions are still caught.
add_executable(stats stats.cpp)
# set_target_properties(stats PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(stats dd99::allocators)

add_executable(compilation_instantiation compilation_instantiation.cpp)
# set_target_properties(compilation_instantiation PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(compilation_instantiation dd99::allocators)
add_test(NAME compilation_instantiation COMMAND compilation_instantiation)

add_executable(new_syntax new_syntax.cpp)
# set_target_properties(compilation_instantiation PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(new_syntax dd99::allocators)
add_test(NAME new_syntax COMMAND new_syntax)

add_executable(buddy_allocator_algorithms_2 buddy_allocator_algorithms_2.cpp)
target_compile_features(buddy_allocator_algorithms_2 PRIVATE cxx_std_23)
add_test(NAME buddy_allocator_algorithms_2 COMMAND buddy_allocator_algorithms_2)
set_tests_properties(buddy_allocator_algorithms_2 PROPERTIES TIMEOUT 120) # ~8s observed single-core; generous margin for slower/shared CI runners

add_executable(basic_buddy_allocator_test basic_buddy_allocator_test.cpp)
target_link_libraries(basic_buddy_allocator_test dd99::allocators)
# target_compile_features(basic_buddy_allocator_test PRIVATE cxx_std_23)
add_test(NAME basic_buddy_allocator_test COMMAND basic_buddy_allocator_test)

add_executable(any_block_allocator_test any_block_allocator_test.cpp)
target_link_libraries(any_block_allocator_test dd99::allocators)
# target_compile_features(any_block_allocator_test PRIVATE cxx_std_23)
add_test(NAME any_block_allocator_test COMMAND any_block_allocator_test)

add_executable(test_assert test_assert.cpp)
target_link_libraries(test_assert dd99::allocators)
target_compile_features(test_assert PRIVATE cxx_std_23)
add_test(NAME test_assert COMMAND test_assert)

add_executable(buddy_allocator_test_2 buddy_allocator_test_2.cpp)
target_link_libraries(buddy_allocator_test_2 dd99::allocators)
target_compile_features(buddy_allocator_test_2 PRIVATE cxx_std_23)
add_test(NAME buddy_allocator_test_2 COMMAND buddy_allocator_test_2)

add_executable(buddy_internal_state_test buddy_internal_state_test.cpp)
target_link_libraries(buddy_internal_state_test dd99::allocators)
target_compile_features(buddy_internal_state_test PRIVATE cxx_std_23)
add_test(NAME buddy_internal_state_test COMMAND buddy_internal_state_test)

# add_executable(buddy_internal_state_test_2 buddy_internal_state_test_2.cpp)
# target_link_libraries(buddy_internal_state_test_2 dd99::allocators)
# target_compile_features(buddy_internal_state_test_2 PRIVATE cxx_std_23)

add_executable(compile_array_alignment_static_assertions compile_array_alignment_static_assertions.cpp)
target_link_libraries(compile_array_alignment_static_assertions dd99::allocators)
target_compile_features(compile_array_alignment_static_assertions PRIVATE cxx_std_23)
add_test(NAME compile_array_alignment_static_assertions COMMAND compile_array_alignment_static_assertions)
