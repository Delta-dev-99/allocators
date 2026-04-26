#pragma once

#include <allocators/internal/bases/buddy.hpp>


namespace dd99::memory::block_allocator
{

    template <std::size_t BLOCK_SIZE,
              unsigned LEVELS,
              class Bitmap_Block_Type = std::byte>
    class Buddy
        : public dd99::memory::block_allocator::internal::base::Buddy_Base<BLOCK_SIZE, LEVELS, Bitmap_Block_Type>
    {
    protected: // internal type definitions
        using Buddy_Base = dd99::memory::block_allocator::internal::base::Buddy_Base<BLOCK_SIZE, LEVELS, Bitmap_Block_Type>;
        using typename Buddy_Base::BMP;

    public: // constant definitions
        using Buddy_Base::Levels;
        using Buddy_Base::Block_Size;
        using Buddy_Base::Max_Block_Size;

    public: // static functions
        static constexpr
        std::size_t calculate_block_count(std::size_t memory_size)
        {
            // check if memory is not enough for 1 block + minimal bitmap
            if (memory_size < BMP::Block_Size + Block_Size)
                return 0;

            std::size_t block_count = memory_size / Block_Size;

            // Iteratively approach block count
            while (true)
            {
                const auto bmp_bits = Buddy_Base::calculate_buddy_bit_count(block_count);
                const auto bmp_size = BMP::calculate_block_count(bmp_bits) * BMP::Block_Size;
                const auto next_block_count = (memory_size - bmp_size) / Block_Size;
                if (next_block_count - block_count <= BMP::calculate_block_count(Levels))
                    break;

                block_count = next_block_count;
            }

            return block_count;
        }

    private: // internal constructors
        Buddy(memory::Block memory, std::size_t block_count, std::byte * blocks_base)
            : Buddy_Base(memory, block_count, memory.base)
            , m_blocks_base(blocks_base)
        {
            deallocate_all();
        }

        Buddy(memory::Block memory, std::size_t block_count)
            : Buddy(memory, block_count, memory.get_end() - block_count * Block_Size)
        { }

    public: // constructors
        Buddy(memory::Block memory)
            : Buddy(memory, calculate_block_count(memory.size))
        { }

        Buddy(const Buddy &) = delete;
        void operator=(const Buddy &) = delete;

    public: // allocator interface implementation
        using Buddy_Base::allocate;
        using Buddy_Base::deallocate;
        using Buddy_Base::deallocate_all;
        using Buddy_Base::owns;

    protected:
        constexpr
        std::byte *
        get_blocks_base() const override
        {
            return m_blocks_base;
        }

    private:
        std::byte * m_blocks_base;
    };

}