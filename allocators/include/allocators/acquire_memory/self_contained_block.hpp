#pragma once

#include <allocators/structures/memory_block.hpp>

namespace dd99::memory
{
    // Contains the memory it describes.
    // Use case: create instance on the stack to acquire stack memory.
    // Use case: create instance on the heap (with new) to acquire heap memory.
    template <std::size_t Size>
    struct Self_Contained_Block : Block
    {
        static_assert(Size > 0);

        constexpr
        Self_Contained_Block()
            : Block{.base = m_data, .size = Size}
        { }

        // No copy
        Self_Contained_Block(const Self_Contained_Block& other) = delete;
        // No move
        Self_Contained_Block(Self_Contained_Block&& other) = delete;

        std::byte m_data[Size];
    };
} // namespace dd99::memory
