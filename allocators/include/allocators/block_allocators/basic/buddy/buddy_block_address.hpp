#pragma once

#include <cstddef>
#include <cstdint>



namespace dd99::memory::block_allocator::buddy_namespace
{

    // This structure encodes how a block is addressed in the buddy allocator
    // The default types use 32-bit level and index types which allow for up to 2^32 blocks on the lower level. assuming 4KB blocks, this gives 4*1024*2^32 = 2^44 bytes, which is 16TB of memory, which is more than enough for most applications. If you need to manage more memory, you can use 64-bit types, but this will increase the size of the block address structure and may have performance implications.
    // Using smaller types may allow for smaller bookkeeping data structures, but may not have the expected benefits due to alignment padding.
    template <class Level_Type = std::uint32_t, class Index_Type = std::uint32_t>
    struct buddy_block_address
    {
        using level_type = Level_Type;
        using index_type = Index_Type;

        level_type level;
        index_type index;
    };

}
