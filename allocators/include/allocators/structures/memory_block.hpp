#pragma once

#include <cstdint>
#include <utility>
#include <cstddef>


namespace dd99::memory
{
    // A block of memory.
    // Described by base address and size in bytes.
    struct Block
    {
        std::byte * base = nullptr;
        std::size_t size = 0;

        constexpr
        std::byte *
        get_end() const { return base + size; }
        
        constexpr
        bool
        contains(const Block & other) const
        {
            const auto base_offset = other.base - base;
            return (base_offset >= 0) && (base + base_offset + other.size <= get_end());
        }

        constexpr
        bool contains(std::byte * ptr) const
        {
            return (base <= ptr) && (get_end() >= ptr);
        }

        // check if block is empty
        constexpr
        operator bool() const
        {
            return size;
        }
    };

    

}
