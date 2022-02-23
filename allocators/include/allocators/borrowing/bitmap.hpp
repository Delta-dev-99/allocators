#pragma once

#include <allocators/basic/allocator.hpp>
#include <allocators/structures/bitmap.hpp>


// borrowing allocators borrow memory from other allocators to place inner state
namespace dd99::memory::block_allocator::borrowing
{
    template <std::size_t Block_Size, class Bitmap_Element_T = std::uint8_t>
    class Bitmap : public Allocator
    {
        using BMP_Structure = dd99::memory::structure::Bitmap<Bitmap_Element_T>;

    public: // statics
        static constexpr std::size_t calculate_aux_allocation(std::size_t memory_size)
        {
            // block count is also the required number of bits
            // one bit per block
            const auto block_count = memory_size / Block_Size;
            return BMP_Structure::Block_Size * BMP_Structure::calculate_block_count(block_count);
        }

    public: // constructors
        ~Bitmap()
        {
            m_aux_allocator.deallocate(m_aux_memory);
        }

        Bitmap(const memory::Block & memory, Allocator & aux_allocator)
            : m_block_count(memory.size / Block_Size)
            , m_aux_allocator(aux_allocator)
            , m_memory(memory)
            , m_aux_memory(m_aux_allocator.allocate(calculate_aux_allocation(memory.size)))
            , m_bitmap(m_block_count, m_aux_memory.base)
        {
            // NOTE: This is safe because the bitmap structure does not operate on the memory during construction
            if (!m_aux_memory)
                throw std::runtime_error{"Borrowed Bitmap Allocator initialization: Auxiliary allocation failed"};
        }
    public: // allocator interface
        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size)
        {
            // larger allocations not supported
            if (requested_size > Block_Size)
                return {};

            const auto free_index = m_bitmap.set_first_unset();
            if (free_index != -1)
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

        bool owns(void *memory) const
        {
            return m_memory.contains(memory);
        }

    private:
        // traduce index to memory block
        Block get_memory_block(std::size_t index) const
        {
            const auto block_offset = index * Block_Size;
            const auto block_base = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(m_memory.base) + block_offset);
            return {.base = block_base, .size = Block_Size};
        }

        // traduce memory block to bitmap index
        std::size_t get_index(const memory::Block& blk) const
        {
            const auto block_offset = reinterpret_cast<std::uintptr_t>(blk.base) - reinterpret_cast<std::uintptr_t>(m_memory.base);
            return block_offset / Block_Size;
        }

    private:
        std::size_t m_block_count;
        Allocator & m_aux_allocator;
        memory::Block m_memory, m_aux_memory;
        BMP_Structure m_bitmap;
    };
}
