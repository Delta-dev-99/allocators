
add_library(dd99_allocators_freestanding INTERFACE)
add_library(dd99::allocators_freestanding ALIAS dd99_allocators_freestanding)
target_compile_features(dd99_allocators_freestanding INTERFACE cxx_std_26)
target_include_directories(dd99_allocators_freestanding INTERFACE include)
# set_target_warnings(dd99_allocators_freestanding WARNINGS_AS_ERRORS PRIVATE)

# target_sources(allocators PRIVATE
#     src/allocators.cpp
# )

add_library(dd99_allocators INTERFACE)
add_library(dd99::allocators ALIAS dd99_allocators)
target_link_libraries(dd99_allocators INTERFACE dd99::allocators_freestanding)

target_sources(dd99_allocators INTERFACE
    src/assertions.cpp

    # include/allocators/allocator.hpp
    # include/allocators/allocators.hpp
    # include/allocators/exception.hpp

    # include/allocators/internal/structures/bitmap.hpp
    # include/allocators/internal/structures/buddy_freelist_array.hpp
    # include/allocators/internal/structures/free_list.hpp

    # include/allocators/internal/bases/buddy.hpp

    # include/allocators/structures/memory_block.hpp

    # include/allocators/basic/bitmap.hpp
    # include/allocators/basic/buddy.hpp
    # include/allocators/basic/pool.hpp
    # include/allocators/basic/slicing.hpp
    # include/allocators/basic/stack.hpp
    
    # include/allocators/borrowing/bitmap.hpp
    # include/allocators/borrowing/buddy.hpp
    
    # include/allocators/composite/quantizer.hpp
    # include/allocators/composite/fallback.hpp
    # include/allocators/composite/segregator.hpp
    # include/allocators/composite/ref.hpp
    # include/allocators/composite/throwing.hpp

    # include/allocators/degenerate/boolean.hpp
    # include/allocators/degenerate/constant.hpp
    # include/allocators/degenerate/null.hpp
    
    # include/allocators/metrics/stats.hpp
    # include/allocators/metrics/timing.hpp

    # include/allocators/pointer/basic.hpp
    # include/allocators/pointer/checked.hpp

    # include/allocators/utility/owner.hpp
    # include/allocators/utility/unique_block.hpp
)
