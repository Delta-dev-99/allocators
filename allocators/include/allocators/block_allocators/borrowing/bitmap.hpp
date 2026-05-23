#pragma once

#include <allocators/block_allocators/block_allocator.hpp>
#include <allocators/block_allocators/degenerate/constant.hpp>
#include <allocators/block_allocators/composite/throwing.hpp>
#include <allocators/block_allocators/utility/unique_block.hpp>
#include <allocators/block_allocators/internal/structures/bitmap.hpp>
#include <allocators/alignment.hpp>


// borrowing allocators borrow memory from other allocators to place inner state
namespace dd99::memory::block_allocator::borrowing
{
    template <std::size_t Block_Size,
              class Sub_Alloc_T = dd99::memory::block_allocator::degenerate::Constant,
              std::size_t Block_Alignment = Block_Size,
              class Bitmap_Element_T = std::uint8_t>
    class Bitmap
    {
        using BMP_Structure = dd99::memory::structure::Bitmap<Bitmap_Element_T>;

        using Aux_Allocator = 
            dd99::memory::block_allocator::utility::Unique_Block_Allocator<
                dd99::memory::block_allocator::composite::Throwing<Sub_Alloc_T>>;
        using Aux_Block = typename Aux_Allocator::Block_Type;


        static_assert(Block_Size > 0,
            "Block_Size must be greater than 0");
        static_assert(Block_Alignment > 0,
            "Block_Alignment must be greater than 0");
        static_assert((Block_Alignment & (Block_Alignment - 1)) == 0,
            "Block_Alignment must be a power of 2");
        static_assert(Block_Size % Block_Alignment == 0,
            "Block_Size must be a multiple of Block_Alignment; "
            "this ensures every block index preserves the alignment guarantee");
        static_assert(std::is_unsigned_v<Bitmap_Element_T>,
            "Bitmap_Element_T must be an unsigned integer type");


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

    private:
        static constexpr memory::Block align_memory(const memory::Block & memory)
        {
            const auto aligned_base = align_up(memory.base, Block_Alignment);
            const auto padding_size = aligned_base - memory.base;
            return memory::Block{.base = aligned_base, .size = memory.size - padding_size};
        }

    public: // constructors
        Bitmap(const memory::Block & memory, Sub_Alloc_T && aux_allocator)
            : m_memory(align_memory(memory))
            , m_block_count(m_memory.size / Block_Size)
            , m_aux_allocator(std::move(aux_allocator))
            , m_aux_memory(m_aux_allocator.allocate(calculate_aux_allocation(m_memory)))
            , m_bitmap(m_block_count, m_aux_memory.base)
        { }
        
    public: // allocator interface
        // Allocate one block.
        //
        // Returns an empty Block if:
        //   - requested_size > Block_Size  (too large for this allocator)
        //   - requested_alignment > Block_Alignment  (guarantee unachievable)
        //   - no free block remains
        //
        // requested_alignment must be a power of 2; behaviour for other values
        // is undefined (matching the C++ standard's own precondition on alignment).
        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size,
                               std::size_t requested_alignment = Block_Alignment)
        {
            if (requested_size > Block_Size) return {};
            if (requested_alignment > Block_Alignment) return {};

            const auto free_index = m_bitmap.set_first_unset();
            if (free_index != std::size_t(-1))
                return get_memory_block(free_index);

            // no free block found
            return {};
        }

        // The caller must pass back the exact Block returned by allocate().
        // Passing a block not owned by this allocator is a no-op.
        void deallocate(const memory::Block& memory)
        {
            if (!owns(memory)) return;
            m_bitmap.unset(get_index(memory));
        }

        void deallocate_all()
        {
            m_bitmap.reset();
        }

        // Containment check. Note: only checks that blk lies within the overall
        // memory range; does not verify block alignment or exact block size.
        // Callers must always pass back the exact Block returned by allocate().
        bool owns(const memory::Block &memory) const
        {
            return m_memory.contains(memory);
        }

        bool owns(const std::byte * memory) const
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
        memory::Block m_memory;
        std::size_t m_block_count;
        Aux_Allocator m_aux_allocator;
        Aux_Block m_aux_memory;
        BMP_Structure m_bitmap;
    };

    static_assert(Block_Allocator<Bitmap<1>>, "This definition doesn't comply with the `Block_Allocator` concept");

}
