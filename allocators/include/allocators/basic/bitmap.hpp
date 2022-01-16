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
        {
            build_bitmap();
        }

    public:
        memory::Block allocate(std::size_t requested_size)
        {
            // larger allocations not supported
            if (requested_size > Block_Size)
                return {};

            
        }

    private:
        memory::Block m_memory;
        std::size_t m_block_count;
        std::size_t m_bitmap_size;
        
        std::uint8_t *get_bitmap_ptr()
        {
            return  reinterpret_cast<std::uint8_t *>(m_memory.base);
        }

        bool bitmap_full()
        {
            return m_block_count % 8 == 0;
        }

        void build_bitmap()
        {
            // worst case n
            m_block_count = (8 * (m_memory.size - Block_Size)) / (8 * Block_Size + 1);
            m_bitmap_size = m_block_count / 8 + (bitmap_full() ? 0 : 1);

            // unused should never be greater than Block_Size
            const auto unused = m_memory.size - m_bitmap_size - m_block_count * Block_Size;
            if ((unused == Block_Size) && !bitmap_full())
                ++m_block_count;


            // reset the bitmap state
            // all blocks set to free
            auto bmp = get_bitmap_ptr();
            for (std::size_t C = 0; C < m_bitmap_size; C++)
            {
                bmp[C] = 0;
            }

            // bits not mapped to blocks
            // set as used
            if (!bitmap_full())
            {
                for (int i = 7; i > (m_block_count % 8); --i)
                {
                    bmp[m_bitmap_size - 1] |= 1 << i;
                }
            }
        }

    };
}
