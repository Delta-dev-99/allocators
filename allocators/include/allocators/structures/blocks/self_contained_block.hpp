#pragma once

#include <allocators/structures/blocks/block_concept.hpp>

namespace dd99::memory
{
    // Contains the memory it describes.
    // Use case: create instance on the stack to acquire stack memory.
    // Use case: create instance on the heap (with new) to acquire heap memory.
    template <std::size_t Size>
    struct self_contained_block
    {
        static_assert(Size > 0);

        constexpr std::byte * get_base() const noexcept { return m_data; }
        constexpr std::size_t get_size() const noexcept { return sizeof(m_data); }
        constexpr std::byte * get_end() const noexcept { return get_base() + get_size(); }
        
        constexpr
        bool
        contains(const block & other) const noexcept
        {
            return (other.get_base() >= get_base()) && (other.get_end() <= get_end());
        }

        constexpr
        bool
        contains(const std::byte * ptr) const noexcept
        {
            return (get_base() <= ptr) && (get_end() >= ptr);
        }

        constexpr
        bool
        empty() const noexcept
        {
            return (get_size() == 0);
        }

        // check if block is empty
        constexpr
        operator bool() const noexcept
        {
            return empty();
        }


        constexpr
        block
        get_block() const noexcept
        {
            return block{
                .base = get_base(),
                .size = get_size()
            };
        }


        std::byte m_data[Size];
    };

    static_assert(Block_Concept<self_contained_block<1>>);

} // namespace dd99::memory
