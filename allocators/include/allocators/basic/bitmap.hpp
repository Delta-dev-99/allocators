#pragma once

#include <allocators/basic/allocator.hpp>
#include <limits>
#include <bit>

// TODO: Read Roaring Bitmaps

namespace dd99::memory::block_allocator
{
    // Allocates fixed-size blocks
    template <std::size_t Block_Size, class Bitmap_Element_T = std::uint8_t>
    class Bitmap : public Allocator
    {
        using BMP_t = Bitmap_Element_T;

    public:
        Bitmap(const memory::Block &memory)
            : m_memory(memory)
        {
            // worst case block count: bitmap is full unused memory is just below 1 block and 1 new bitmap element
            const auto worst_case_unused = Block_Size + sizeof(BMP_t) - 1;
            m_block_count = (n_bits * (m_memory.size - worst_case_unused)) / (n_bits * Block_Size + 1);
            // if bitmap not full, there is a last, partially unused, element that division does not account for.
            m_bitmap_size = m_block_count / n_bits + (bitmap_full() ? 0 : 1);

            // if not worst case, we can use one more block
            const auto unused = m_memory.size - m_bitmap_size * sizeof(BMP_t) - m_block_count * Block_Size;
            if ((unused >= Block_Size) && !bitmap_full())
                ++m_block_count;

            // set the base address of the blocks
            m_blocks = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(m_memory.get_end()) - m_block_count * Block_Size);

            // mark all blocks as free
            deallocate_all();
        }

    public:
        memory::Block allocate(std::size_t requested_size)
        {
            // larger allocations not supported
            if (requested_size > Block_Size)
                return {};
            
            // group bitmap elements (should be of some padding-free unsigned integer type) in blocks for faster iteration
            using Block_t = std::uint_fast32_t;
            const auto n_blocks = m_bitmap_size * sizeof(BMP_t) / sizeof(Block_t);
            auto bmp = get_bitmap_ptr();
            auto block_ptr = reinterpret_cast<Block_t *>(bmp);

            std::size_t block_index = 0;
            for (; block_index < n_blocks; ++block_index)
            {
                if (block_ptr[block_index] != Block_t(-1))
                    break;
            }

            auto index = block_index * sizeof(Block_t) / sizeof(BMP_t);
            for (; index < m_bitmap_size; ++index)
            {
                if (bmp[index] == BMP_t(-1))
                    continue;

                for (int bit = 0; bit < n_bits; ++bit)
                {
                    const auto mask = BMP_t(1) << bit;

                    if ((bmp[index] & mask) == 0)
                    {
                        bmp[index] |= mask;
                        return get_memory_block(index * n_bits + bit);
                    }
                }
            }

            // no free block found
            return {};
        }

        void deallocate(const memory::Block& memory)
        {
            if (!owns(memory))
                return;
            
            // mark block as free
            auto index = get_index(memory);
            get_bitmap_ptr()[index / n_bits] &= ~(BMP_t(1) << (index % n_bits));
        }

        void deallocate_all()
        {
            // reset the bitmap state
            // all blocks set to free
            auto bmp = get_bitmap_ptr();
            for (std::size_t index = 0; index < m_bitmap_size; index++)
            {
                bmp[index] = 0;
            }

            // bits not mapped to blocks
            // set as used
            if (!bitmap_full())
            {
                // number of bits on the last bitmap element that are mapped to memory
                const auto n_last_bits = m_block_count % n_bits;
                const auto n_unused_bits = n_bits - n_last_bits;

                bmp[m_bitmap_size - 1] |= ((BMP_t(1) << n_unused_bits) - 1) << n_last_bits;
            }
        }

        bool owns(const memory::Block &memory) const
        {
            return m_memory.contains(memory);
        }

        bool owns(void *memory) const
        {
            return m_memory.contains(memory);
        }

    private:
        memory::Block m_memory;
        std::size_t m_block_count;
        std::size_t m_bitmap_size; // number of elements in bitmap
        void *m_blocks;
        static constexpr auto n_bits = std::numeric_limits<BMP_t>::digits;
        
        // get ptr to start of bitmap.
        // this function allows to change the bitmap position without affecting logic
        BMP_t *get_bitmap_ptr()
        {
            // bitmap located at the base of the memory
            return  reinterpret_cast<BMP_t *>(m_memory.base);
        }

        // whether all bitmap bits are mapped to memory blocks
        bool bitmap_full() const
        {
            return m_block_count % n_bits == 0;
        }

        // traduce index to memory block
        Block get_memory_block(std::size_t index)
        {
            const auto ptr = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(m_blocks) + Block_Size * index);
            return {.base = ptr, .size = Block_Size};
        }

        // traduce memory block to bitmap index
        std::size_t get_index(const memory::Block& memory)
        {
            return (reinterpret_cast<std::uintptr_t>(memory.base) - reinterpret_cast<std::uintptr_t>(m_blocks)) / Block_Size;
        }
    };
}
