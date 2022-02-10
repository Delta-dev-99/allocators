#pragma once

#include <allocators/basic/allocator.hpp>
#include <allocators/structures/bitmap.hpp>
#include <allocators/structures/free_list.hpp>

#include <cstdint>
#include <tuple>
#include <ranges>

namespace dd99::memory::block_allocator
{

    // namespace detail
    // {
    //     template <std::size_t... Sizes>
    //     struct freelist_collection_complete
    //     {
    //         using type = std::tuple<memory::structure::Freelist_Fixed_Sz_Blocks<Sizes>...>;
    //     };

    //     template <std::size_t BLOCK_SIZE, std::size_t Levels, std::size_t... Sizes>
    //     struct freelist_collection : freelist_collection<BLOCK_SIZE * 2, Levels - 1, Sizes..., BLOCK_SIZE>
    //     { };

    //     template <std::size_t BLOCK_SIZE, std::size_t... Sizes>
    //     struct freelist_collection<BLOCK_SIZE, 0, Sizes...> : freelist_collection_complete<Sizes...>
    //     { };
    // }


    namespace detail
    {
        // TODO: Make constexpr
        // Purpose: Initialize the freelists with apropiate sizes
        template <std::size_t Levels, std::size_t... Sizes>
        class Buddy_Freelist_Impl
        {
        protected:
            memory::structure::Freelist_Fixed_Sz_Blocks m_freelists[Levels];

        public:
            Buddy_Freelist_Impl()
                : m_freelists{Sizes...}
            { }
        };



        template <std::size_t BLOCK_SIZE,
                  std::size_t Levels,
                  std::size_t Current_BLOCK_SIZE = BLOCK_SIZE,
                  std::size_t Remaining_Levels = Levels,
                  std::size_t... Sizes>
        class Buddy_Freelist : public Buddy_Freelist<BLOCK_SIZE, Levels, Current_BLOCK_SIZE * 2, Remaining_Levels - 1, Sizes..., Current_BLOCK_SIZE>
        { };

        template <std::size_t BLOCK_SIZE,
                  std::size_t Levels,
                  std::size_t Current_BLOCK_SIZE,
                  std::size_t... Sizes>
        class Buddy_Freelist<BLOCK_SIZE, Levels, Current_BLOCK_SIZE, 0, Sizes...> : public Buddy_Freelist_Impl<Levels, Sizes...>
        { };
    }


    // NOTE: Max_Block_Size is an inclussive upper bound
    template <std::size_t BLOCK_SIZE = (1 << 12),
              std::size_t LEVELS = 11,
              class Bitmap_Element_T = std::uint8_t>
    class Buddy : public Allocator, public detail::Buddy_Freelist<BLOCK_SIZE, LEVELS>
    {
        using Freelist_Base_t = detail::Buddy_Freelist<BLOCK_SIZE, LEVELS>;

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
            : Freelist_Base_t()
            , m_memory(memory)
            , m_block_count(calculate_block_count(memory.size))
            , m_bitmap(bitmap_bits(m_block_count), memory.base)
        {
            deallocate_all();
        }

    public: // statics
        constexpr static std::size_t bitmap_bits(std::size_t block_count)
        {
            // = block_count/2 + block_count/4 + ... + block_count/(1 << (Levels))
            // One bit per pair. Last level does not need bits
            return ((std::size_t(1) << (Levels - 1) - 1) * block_count) / (std::size_t(1) << (Levels - 1));
        }

        // get the number of blocks in the specified level
        static constexpr std::size_t calculate_block_count(std::size_t memory_size, std::size_t level = 0)
        {
            if (level > 0) return calculate_block_count(memory_size) / (1 << level);

            // Not enough memory to make a block
            if (memory_size < BMP::Block_Size + Block_Size)
                return 0;

            std::size_t n = memory_size / Block_Size;

            // Iteratively aproach block count
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

    public:
        [[nodiscard]]
        memory::Block allocate_from_level(std::size_t requested_level)
        {
            // allocation too large
            if (requested_level >= Levels)
                return {};

            // try get block
            Block blk = Freelist_Base_t::m_freelists[requested_level].pop();

            // failed? Split larger block
            if (!blk)
            {
                // allocate larger block
                blk = allocate_from_level(requested_level + 1);
                // failed? Nothing else we can do
                if (!blk)
                    return {};
                
                // one half is free
                blk.size /= 2;
                Freelist_Base_t::m_freelists[requested_level].push(blk);

                // we use the other
                blk.base = blk.get_end();
            }

            // toggle the corresponding bit
            const auto index = get_bitmap_index(blk, requested_level);
            if (index != std::size_t(-1))
                m_bitmap.toggle(index);

            return blk;
        }

        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size)
        {
            if (requested_size == 0)
                return {};

            const auto n_blocks = (requested_size - 1) / Block_Size + 1;
            const auto requested_level = std::bit_width(n_blocks) - 1;

            return allocate_from_level(requested_level);
        }

        void deallocate(const memory::Block &memory)
        {
            if (!owns(memory))
                return;

            // assumed size of b is exact multiple of Block_Size
            // n_blocks: size in blocks (the smallest ones)
            const auto n_blocks = memory.size / Block_Size;
            const auto block_level = std::bit_width(n_blocks) - 1;

            const auto bmp_index = get_bitmap_index(memory, block_level);
            // check if there is a buddy.
            if (bmp_index == std::size_t(-1))
                return;

            // there is a buddy. Toggle the corresponding bit
            m_bitmap.toggle(bmp_index);

            // Join buddies
            if (m_bitmap[bmp_index] == 0)
                deallocate(get_block(get_block_index_in_lvl(memory, block_level) / 2, block_level + 1));
            else
                Freelist_Base_t::m_freelists[block_level].push(memory);
        }

        void deallocate_all()
        {
            m_bitmap.reset();

            for (int i = 0; i < Levels; i++)
            {
                Freelist_Base_t::m_freelists[i].clear();
            }


            // Build freelists

            // first add all blocks in the last level to freelist
            // NOTE: iterating through blocks in the last level
            for (std::size_t i = 0; i < block_count(Levels - 1); i++)
            {
                Freelist_Base_t::m_freelists[Levels - 1].push(get_block(i, Levels - 1));
            }

            // next add blocks without buddies to corresponding freelists
            // NOTE: iterating through levels (except the last one)
            for (std::size_t l = 0; l < Levels - 1; l++)
            {
                // check if the last block in the level has buddy.
                // add it to freelist if it doesn't have one.
                if (block_count(l) % 2 != 0)
                    Freelist_Base_t::m_freelists[l].push(get_block(block_count(l) - 1, l));
            }
        }

        bool owns(void *memory) const
        {
            return m_memory.contains(memory);
        }

        bool owns(const memory::Block& memory) const
        {
            return m_memory.contains(memory);
        }

    public:
        std::size_t block_count(std::size_t level = 0) const
        {
            return m_block_count / (std::size_t(1) << level);
        }

    private:
        memory::Block m_memory;
        std::size_t m_block_count;
        void *m_blocks_base;
        BMP m_bitmap;
        // typename detail::freelist_collection<Block_Size, Levels>::type m_freelists;

    private:
        std::size_t get_block_index_in_lvl(const Block &b, std::size_t level) const
        {
            // size of blocks on this level
            const auto block_size = Block_Size << level;
            // offset from blocks base
            const auto block_offset = reinterpret_cast<std::uintptr_t>(&b) - reinterpret_cast<std::uintptr_t>(m_blocks_base);
            // the index of the block on the level
            return block_offset / block_size;
        }

        std::size_t get_bitmap_index(const Block &blk, std::size_t level) const
        {
            // blocks in the last level do not have buddies
            // (nor bits)
            if (level + 1 == Levels)
                return -1;

            const auto index_in_lvl = get_block_index_in_lvl(blk, level);
            // check if block has buddy
            // conditions: block is the first in the buddy (otherwise, the buddy exists and is the previous block)
            // conditions: block is the last in the level (otherwise, the buddy exists and is the next block)
            // therefore the buddy cannot exist.
            if ((index_in_lvl % 2 == 0) && (index_in_lvl == block_count(level) - 1))
                return -1;

            // calculate the total number of block pairs in previous levels
            const auto a = std::size_t(1) << level - 1;
            const auto b = std::size_t(1) << level;
            const auto n_prev = (a * m_block_count) / b;

            return n_prev + index_in_lvl / 2;
        }


        Block get_block(std::size_t index_in_lvl, std::size_t level) const
        {
            const auto block_size = Block_Size << level;
            const auto base = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(m_blocks_base) + Block_Size * index_in_lvl);
            return Block{.base = base, .size = block_size};
        }
    };
}


// NOTES:

// TODO:
// Add calculation for optimum location of bitmap and blocks
// Where to leave unused space? Between bitmap and blocks? At the ends? Both? Something else?
// Take into account alignment for blocks (blocks may be of not-power-of-2 sizes)
