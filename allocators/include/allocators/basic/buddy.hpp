#pragma once

#include <allocators/basic/allocator.hpp>
#include <allocators/structures/bitmap.hpp>
#include <allocators/structures/free_list.hpp>

#include <cstdint>
#include <tuple>
#include <ranges>

namespace dd99::memory::block_allocator
{

    namespace detail
    {
        template <std::size_t... Sizes>
        struct freelist_collection_complete
        {
            using type = std::tuple<memory::structure::Freelist_Fixed_Sz_Blocks<Sizes>...>;
        };

        template <std::size_t BLOCK_SIZE, std::size_t Levels, std::size_t... Sizes>
        struct freelist_collection : freelist_collection<BLOCK_SIZE * 2, Levels - 1, Sizes..., BLOCK_SIZE>
        { };

        template <std::size_t BLOCK_SIZE, std::size_t... Sizes>
        struct freelist_collection<BLOCK_SIZE, 0, Sizes...> : freelist_collection_complete<Sizes...>
        { };
    }

    // NOTE: Max_Block_Size is an inclussive upper bound
    template <std::size_t BLOCK_SIZE = (1 << 12),
              std::size_t LEVELS = 11,
              class Bitmap_Element_T = std::uint8_t>
    class Buddy
    {
    public:
        static constexpr std::size_t Levels = LEVELS;
        static constexpr std::size_t Block_Size = BLOCK_SIZE;
        static constexpr std::size_t Max_Block_Size = Block_Size << (Levels - 1);

        static_assert(Levels > 0); // NOTE: For lvl = 1 there is no point.
        static_assert(Levels < std::numeric_limits<std::size_t>::digits); // Just in case...
        static_assert(Block_Size > 0);

        using BMP = dd99::memory::structure::Bitmap<Bitmap_Element_T>;

    public:
        Buddy(const memory::Block &memory)
            : m_memory(memory)
            , m_bitmap(bitmap_bits(block_count(memory.size)), memory.base)
        {
            
        }

    public: // statics
        constexpr static std::size_t bitmap_bits(std::size_t block_count)
        {
            // = block_count + block_count/2 + block_count/4 + ... + block_count/(1 << (Levels-1))
            return (block_count * ((1 << Levels) - 1)) / (1 << (Levels - 1));
        }

        static constexpr std::size_t block_count(std::size_t memory_size, std::size_t level = 0)
        {
            if (level > 0) return block_count(memory_size) / (1 << level);

            // Not enough memory
            if (memory_size < BMP::Block_Size + Block_Size)
                return 0;

            std::size_t n = memory_size / Block_Size;

            while (true)
            {
                const auto bmp_size = BMP::size(bitmap_bits(n));
                const auto next_n = (memory_size - bmp_size) / Block_Size;
                if (next_n - n <= BMP::size(Levels))
                    break;

                n = next_n;
            }

            return n;
        }

        static constexpr double ratio(std::size_t n_blocks)
        {
            return double(n_blocks * Block_Size)/(BMP::size(bitmap_bits(n_blocks)) * BMP::Block_Size);
        }

    private:
        memory::Block m_memory;
        std::size_t n_blocks[Levels];
        BMP m_bitmap;
        typename detail::freelist_collection<Block_Size, Levels>::type m_freelists;

    private:
        

        // constexpr std::array<dd99::memory::structure::Bitmap<std::uint_fast8_t>, Levels>

        

        // constexpr static auto full_block_count(std::size_t memory_size)
        // {
            
        // }

        // void * get_bitmap_base(std::size_t level)
        // {

        // }
    };
}
