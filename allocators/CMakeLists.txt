
add_library(allocators)
target_compile_features(allocators PUBLIC cxx_std_20)
target_include_directories(allocators PUBLIC include)

target_sources(allocators PRIVATE
    src/allocators.cpp
)

target_sources(allocators PUBLIC
    include/allocators/structures/free_list.hpp
    include/allocators/structures/memory_block.hpp

    include/allocators/basic/allocator.hpp
    include/allocators/basic/slicing.hpp
    include/allocators/basic/pool.hpp
    include/allocators/basic/stack.hpp
    include/allocators/basic/bitmap.hpp
    include/allocators/basic/buddy.hpp
    
    include/allocators/composite/bucketizer.hpp
    include/allocators/composite/fallback.hpp
    include/allocators/composite/ref.hpp
    include/allocators/composite/segregator.hpp
    include/allocators/composite/stats.hpp
    include/allocators/composite/timing.hpp

    include/allocators/pointer/allocator.hpp
    include/allocators/pointer/basic.hpp
    include/allocators/pointer/checked.hpp
)
