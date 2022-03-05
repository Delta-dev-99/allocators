#pragma once

#include <allocators/basic/allocator.hpp>
#include <allocators/degenerate/constant.hpp>
#include <allocators/utility/throwing.hpp>
#include <allocators/utility/unique_block.hpp>
#include <allocators/internal_structures/bitmap.hpp>


// borrowing allocators borrow memory from other allocators to place inner state
namespace dd99::memory::block_allocator::borrowing
{
    template <std::size_t Block_Size,
              class Sub_Alloc_T = dd99::memory::block_allocator::degenerate::Constant,
              class Bitmap_Element_T = std::uint8_t>
    class Bitmap : public Allocator
    {
        using BMP_Structure = dd99::memory::structure::Bitmap<Bitmap_Element_T>;

        using Aux_Allocator = 
            dd99::memory::block_allocator::composite::Unique_Block_Allocator<
                dd99::memory::block_allocator::composite::Throwing<Sub_Alloc_T>>;
        using Aux_Block = typename Aux_Allocator::Block_Type;

    public: // statics
        static constexpr std::size_t calculate_aux_allocation(std::size_t block_count)
        {
            // A bitmap with one bit per block
            return BMP_Structure::Block_Size * BMP_Structure::calculate_block_count(block_count);
        }

        static constexpr std::size_t calculate_aux_allocation(const memory::Block & memory)
        {
            return calculate_aux_allocation(memory.size / Block_Size);
        }

    public: // constructors
        Bitmap(const memory::Block & memory, Sub_Alloc_T && aux_allocator)
            : m_block_count(memory.size / Block_Size)
            , m_aux_allocator(std::move(aux_allocator))
            , m_memory(memory)
            , m_aux_memory(m_aux_allocator.allocate(calculate_aux_allocation(memory)))
            , m_bitmap(m_block_count, m_aux_memory.base)
        { }
        
    public: // allocator interface
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

    private:
        // traduce index to memory block
        Block get_memory_block(std::size_t index) const
        {
            const auto block_offset = index * Block_Size;
            const auto block_base = m_memory.base + block_offset;
            return {.base = block_base, .size = Block_Size};
        }

        // traduce memory block to bitmap index
        std::size_t get_index(const memory::Block& blk) const
        {
            const auto block_offset = std::size_t(blk.base - m_memory.base);
            return block_offset / Block_Size;
        }

    private:
        std::size_t m_block_count;
        Aux_Allocator m_aux_allocator;
        memory::Block m_memory;
        Aux_Block m_aux_memory;
        BMP_Structure m_bitmap;
    };
}
