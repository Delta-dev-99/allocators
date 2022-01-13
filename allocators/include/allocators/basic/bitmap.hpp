#pragma once

#include <allocators/basic/allocator.hpp>

// TODO: Read Roaring Bitmaps

namespace dd99::memory::block_allocator
{
    // Allocates fixed-size blocks 
    template <std::size_t Block_Size>
    class Bitmap : public Allocator
    {
    public:
        Bitmap(const memory::Block &memory)
            : m_memory(memory)
        { }

    public:
        memory::Block allocate(std::size_t requested_size)
        {
            // larger allocations not supported
            if (requested_size > Block_Size)
                return {};

            
        }

    private:
        memory::Block m_memory;
        std::uint8_t *m_bitmap;
    };
}
