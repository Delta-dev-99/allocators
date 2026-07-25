#pragma once

#include <allocators/library_configuration/cpp_config.hpp>
#include <allocators/structures/blocks/block_concept.hpp>
#include <allocators/structures/bitmap.hpp>
#include <allocators/alignment.hpp>
#include <utility>



namespace dd99_allocators_namespace::block_allocator
{

    // allocates fixed-size blocks,
    // tracking free/used state with a bitmap
    // 
    // *** template parameters:
    // Block_Size:                  size in bytes of allocatable blocks
    // Block_Alignment:             alignment guarantee for each block
    // Bitmap_Element_Type:         type used as bitmap word
    // Managed_Memory_Block_Type:   type for the managed memory block
    // State_Memory_Block_Type:     type for the state memory block (bitmap)
    // 
    // requirements:
    // managed memory base is aligned to Block_Alignment
    // Block_Alignment is a divisor of Block_Size (to hold alignment guarantee)
    // state memory base is properly aligned for Bitmap_Element_Type
    // state memory is big enough for the bitmap, given managed memory. can check using static functions
    // Bitmap_Element_Type is appropriate, according to the bitmap data structure (`bmp_type`)
    template <std::size_t Block_Size,
              std::size_t Block_Alignment = Block_Size,
              class Bitmap_Element_Type = std::byte,
              Block_Concept Managed_Memory_Block_Type = block,
              Block_Concept State_Memory_Block_Type = block>
    class bitmap
    {
        using bmp_element_type = Bitmap_Element_Type;
        using bmp_type = dd99_allocators_namespace::structure::Bitmap<bmp_element_type>;
        
        using managed_memory_block_type = Managed_Memory_Block_Type;
        using state_memory_block_type = State_Memory_Block_Type;

        static constexpr auto block_size = Block_Size;
        static constexpr auto block_alignment = Block_Alignment;

        static_assert(block_size > 0,
            "block_size must be greater than 0");
        static_assert(block_alignment > 0,
            "block_alignment must be greater than 0");
        static_assert((block_alignment & (block_alignment - 1)) == 0,
            "block_alignment must be a power of 2");
        static_assert(block_size % block_alignment == 0,
            "block_size must be a multiple of block_alignment; "
            "this ensures every block index preserves the alignment guarantee");
        // static_assert(std::is_unsigned_v<bmp_element_type>,
            // "bmp_element_type must be an unsigned integer type");

    public:
        static constexpr
        std::size_t
        calculate_block_count(block blk)
        {
            auto aligned_base = align_up(blk.get_base(), block_alignment);
            if (aligned_base > blk.get_end()) return 0; // safeguard against evil alignments
            auto aligned_size = static_cast<std::size_t>(blk.get_end() - aligned_base);
            return aligned_size / block_size;
        }

    public:
        constexpr bitmap(managed_memory_block_type managed_mem, state_memory_block_type state_mem)
            : m_managed_memory{std::move(managed_mem)}
            , m_state_memory{std::move(state_mem)}
            , m_block_count{calculate_block_count(m_managed_memory)}
            , m_bmp{m_block_count, m_state_memory.get_base()}
        {
            DD99_ALLOCATORS_ASSERT_HARDENED("managed memory base must be aligned to Block_Alignment", is_aligned(m_managed_memory.get_base(), block_alignment));
            DD99_ALLOCATORS_ASSERT_HARDENED("state memory base must be properly aligned", is_aligned(m_state_memory.get_base(), bmp_type::block_alignment));
            DD99_ALLOCATORS_ASSERT_CRITICAL("state memory block must be big enough", m_state_memory.get_size() >= bmp_type::calculate_block_count(m_block_count) * bmp_type::block_size);
            
            deallocate_all();
        }

        constexpr bitmap(const bitmap &) = delete;
        constexpr bitmap & operator=(const bitmap &) = delete;
        
        constexpr bitmap(bitmap &&) = default;
        constexpr bitmap & operator=(bitmap &&) = default;

    public:
        [[nodiscard]]
        constexpr block
        allocate(std::size_t requested_size, std::size_t requested_alignment = block_alignment)
        {
            if (requested_size      > Block_Size)      return {};
            if (requested_alignment > Block_Alignment) return {};

            const auto free_index = m_bmp.set_first_unset();
            if (free_index != std::size_t(-1))
            {
                return get_memory_block(free_index);
            }
            else
            {
                return {};
            }
        }

        constexpr
        void
        deallocate(block blk)
        {
            if (!owns(blk)) return;
            m_bmp.unset(get_index(blk));
        }

        constexpr
        void
        deallocate_all()
        {
            m_bmp.reset();
        }

        constexpr
        bool
        owns(block blk) const noexcept
        {
            return m_managed_memory.contains(blk);
        }

        constexpr
        bool
        owns(const std::byte * ptr) const noexcept
        {
            return m_managed_memory.contains(ptr);
        }

    private:
        [[nodiscard]]
        constexpr
        block
        get_memory_block(std::size_t index) const noexcept
        {
            auto offset = block_size * index;
            auto block_base = m_managed_memory.get_base() + offset;
            return block{
                .base = block_base,
                .size = block_size
            };
        }

        [[nodiscard]]
        constexpr
        std::size_t
        get_index(block blk) const noexcept
        {
            auto offset = blk.get_base() - m_managed_memory.get_base();
            return static_cast<std::size_t>(offset / block_size);
        }


        managed_memory_block_type m_managed_memory;
        state_memory_block_type m_state_memory;
        std::size_t m_block_count;
        bmp_type m_bmp;
    };

}
