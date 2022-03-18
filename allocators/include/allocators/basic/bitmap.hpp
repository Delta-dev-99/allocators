#pragma once

#include <allocators/internal/bases/allocator.hpp>
#include <allocators/internal/structures/bitmap.hpp>

#include <limits>
#include <bit>

// TODO: Read Roaring Bitmaps

namespace dd99::memory::block_allocator
{
    // Allocates fixed-size blocks
    // Uses some of the memory to keep a bitmap
    template <std::size_t Block_Size, class Bitmap_Element_T = std::uint8_t>
    class Bitmap : public Allocator
    {
        using BMP = dd99::memory::structure::Bitmap<Bitmap_Element_T>;

    public:
        Bitmap(const memory::Block &memory)
            : m_memory(memory)
            , m_block_count(block_count(memory.size))
            , m_blocks_base(m_memory.get_end() - m_block_count * Block_Size)
            , m_bitmap(m_block_count, memory.base)
        {
            // mark all blocks as free
            deallocate_all();
        }

    public:
        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size)
        {
            // larger allocations not supported
            if (requested_size > Block_Size)
                return {};

            const auto free_index = m_bitmap.set_first_unset();
            if (free_index != std::size_t(-1))
                return get_memory_block(free_index);

            // no free block found
            return {};
        }

        void deallocate(const memory::Block& memory)
        {
            if (!owns(memory))
                return;
            
            // mark block as free
            auto index = get_index(memory);
            m_bitmap.unset(index);
        }

        void deallocate_all()
        {
            m_bitmap.reset();
        }

        bool owns(const memory::Block &memory) const
        {
            return m_memory.contains(memory);
        }

        bool owns(std::byte *memory) const
        {
            return m_memory.contains(memory);
        }

    public:
        constexpr static auto block_count(std::size_t memory_size)
        {
            // worst case block count: bitmap is full unused memory is just below 1 block and 1 new bitmap element
            const auto worst_case_unused = Block_Size + BMP::Block_Size - 1;
            // TODO: This formula is wrong
            auto block_count = (BMP::Block_Bits * (memory_size - worst_case_unused)) / (BMP::Block_Bits * Block_Size + 1);
            const auto unused = memory_size - BMP::calculate_block_count(block_count) * BMP::Block_Size - block_count * Block_Size;
            
            if (unused >= Block_Size && !BMP::fully_mapped(block_count))
                ++block_count;
            
            return block_count;
        }

    private:
        memory::Block m_memory;
        std::size_t m_block_count;
        // std::size_t m_bitmap_size; // number of elements in bitmap
        std::byte *m_blocks_base;
        // static constexpr auto n_bits = std::numeric_limits<BMP_t>::digits;

        BMP m_bitmap;



        // traduce index to memory block
        Block get_memory_block(std::size_t index)
        {
            const auto ptr = m_blocks_base + Block_Size * index;
            return {.base = ptr, .size = Block_Size};
        }

        // traduce memory block to bitmap index
        std::size_t get_index(const memory::Block& memory)
        {
            return std::size_t(memory.base - m_blocks_base) / Block_Size;
        }
    };
}
