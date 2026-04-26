#pragma once

#include <allocators/internal/bases/buddy.hpp>
#include <cassert>


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

            // Analytical initial guess from continuous relaxation
            constexpr auto pow2_Lm1 = 1ULL << (Levels - 1);
            constexpr auto a_num = pow2_Lm1 - 1;          // 2^{L-1} - 1
            constexpr auto denom = 8ULL * Block_Size * pow2_Lm1 + a_num;
            // Numerator: 8 * memory_size * pow2_Lm1
            // We assume memory_size * 8 * pow2_Lm1 < 2^64 (holds for realistic RAM).

            // Ensure memory_size doesn't cause overflow: memory_size * 8 * pow2_Lm1 < 2^64
            constexpr auto max_supported_memory = std::numeric_limits<std::size_t>::max() / (8ULL * pow2_Lm1);
            assert(memory_size <= max_supported_memory && "memory_size would cause overflow in calculation");
            
            // Calculate initial block count guess
            std::size_t block_count = (8ULL * memory_size * pow2_Lm1) / denom;

            // helper predicate
            auto fits = [&](std::size_t block_count) -> bool {
                std::size_t bits = Buddy_Base::calculate_buddy_bit_count(block_count);
                std::size_t bitmap = BMP::calculate_block_count(bits) * BMP::Block_Size;
                return block_count * Block_Size + bitmap <= memory_size;
            };

            // adjust - at most ceil(BMP::Block_Size / Block_Size) steps
            if (fits(block_count))
            {
                while(fits(block_count + 1)) ++block_count;
            }
            else
            {
                while(!fits(--block_count)); // decrement until fits
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