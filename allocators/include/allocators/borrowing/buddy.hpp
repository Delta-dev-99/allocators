#pragma once

#include <allocators/basic/allocator.hpp>
#include <allocators/structures/bitmap.hpp>


// borrowing allocators borrow memory from other allocators to place inner state
namespace dd99::memory::block_allocator::borrowing
{

    namespace detail
    {

        // TODO: Make constexpr
        // Purpose: Initialize the freelists with apropiate sizes
        template <int Levels, std::size_t... Sizes>
        class Buddy_Freelist_Impl
        {
        protected:
            memory::structure::Freelist m_freelists[Levels];

        public:
            Buddy_Freelist_Impl()
                : m_freelists{Sizes...}
            { }
        };



        template <std::size_t BLOCK_SIZE,
                  int Levels,
                  std::size_t Current_BLOCK_SIZE = BLOCK_SIZE,
                  std::size_t Remaining_Levels = Levels,
                  std::size_t... Sizes>
        class Buddy_Freelist : public Buddy_Freelist<BLOCK_SIZE, Levels, Current_BLOCK_SIZE * 2, Remaining_Levels - 1, Sizes..., Current_BLOCK_SIZE>
        { };

        template <std::size_t BLOCK_SIZE,
                  int Levels,
                  std::size_t Current_BLOCK_SIZE,
                  std::size_t... Sizes>
        class Buddy_Freelist<BLOCK_SIZE, Levels, Current_BLOCK_SIZE, 0, Sizes...> : public Buddy_Freelist_Impl<Levels, Sizes...>
        { };

    }



    template <std::size_t BLOCK_SIZE = (1 << 12),
              int LEVELS = 11,
              class Bitmap_Element_T = std::uint8_t>
    class Buddy : public Allocator, public detail::Buddy_Freelist<BLOCK_SIZE, LEVELS>
    {
        using Freelist_Base = detail::Buddy_Freelist<BLOCK_SIZE, LEVELS>;
        using BMP_Structure = dd99::memory::structure::Bitmap<Bitmap_Element_T>;

    public: // static consts
        static constexpr int Levels = LEVELS;
        static constexpr std::size_t Block_Size = BLOCK_SIZE;
        static constexpr std::size_t Max_Block_Size = Block_Size << (Levels - 1);
        
        static_assert(Levels > 0); // NOTE: For lvl = 1 there is no point.
        static_assert(Levels < std::numeric_limits<std::size_t>::digits); // Just in case...
        static_assert(Block_Size > 0);

    public: // statics
        static constexpr std::size_t calculate_aux_allocation(std::size_t memory_size)
        {
            const auto first_level_block_count = memory_size / Block_Size;

            // calculate bitmap bit count
            // 1 bit per buddy relation
            // last level does not need bits
            std::size_t bit_count = 0;
            for (int i = 0; i < Levels - 1; i++)
            {
                bit_count += first_level_block_count >> (std::size_t(1) << (i + 1));
            }

            const auto bmp_blocks = BMP_Structure::calculate_block_count(bit_count);
            const auto bmp_bytes = bmp_blocks * BMP_Structure::Block_Size;
            return bmp_bytes;
        }

    public:
        ~Buddy()
        {
            m_aux_allocator.deallocate(m_aux_memory);
        }

        Buddy(const memory::Block & memory, Allocator & aux_allocator)
            : Freelist_Base()
            , m_memory(memory)
            , m_block_count(memory.size / Block_Size)
            , m_aux_allocator(aux_allocator)
            , m_aux_memory(m_aux_allocator.allocate(calculate_aux_allocation(memory.size)))
            , m_bitmap(m_block_count, m_aux_memory.base)
        {
            // NOTE: This is safe because the bitmap structure does not operate on the memory during construction
            if (!m_aux_memory)
                throw std::runtime_error{"Buddy Allocator: Borrowed allocator initialization: Auxiliary allocation failed"};
        }

    public:
        [[nodiscard]]
        memory::Block allocate_from_level(int requested_level)
        {
            // allocation too large?
            if (requested_level >= Levels)
                return {};

            // step 1: get the block
            // try to get a block from the freelist for the current level.
            // if that succeeds, we are done.
            // otherwise, allocate a block from the next level and split it.
            // if the allocation fails there's nothing else we can do.
            // the first half is put in the freelist of the current level.
            // the other half is what we use.

            Block blk = Freelist_Base::m_freelists[requested_level].pop();
            if (!blk)
            {
                blk = allocate_from_level(requested_level + 1);
                if (!blk) return {};

                blk.size /= 2;
                Freelist_Base::m_freelists[requested_level].push(blk);

                blk.base = blk.get_end();
            }

            // step 2: got a block, toggle the buddy bit.
            // some blocks do not have a buddy (no bit then).
            // this is the case for the last block on the level if there is an odd number of them

            const auto bmp_index = get_bitmap_index(blk, requested_level);
            if (bmp_index != -1)
                m_bitmap.toggle(bmp_index);

            return blk;
        }

        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size)
        {
            // Find out what level the allocation requires
            // relay to the function that allocates from levels

            if (requested_size == 0)
                return {};

            // ceiling division of requested_size / Block_Size
            const auto number_of_blocks_required = (requested_size - 1) / Block_Size + 1;
            const int required_block_level = std::bit_width(number_of_blocks_required) - 1;

            return allocate_from_level(required_block_level);
        }

        void deallocate(const memory::Block & memory)
        {
            // NOTE: it is assumed the memory block was not modified.
            // NOTE: the block size is assumed to be exactly the size of blocks from some level.

            if (!owns(memory))
                return;

            const auto number_of_blocks = memory.size / Block_Size;
            // NOTE: This can be optimized
            const int block_level = std::bit_width(number_of_blocks) - 1;

            const auto bmp_index = get_bitmap_index(memory, block_level);
            if (bmp_index != -1)
            {
                m_bitmap.toggle(bmp_index);
                if (m_bitmap[bmp_index] == 0)
                {
                    // both blk and its buddy are free.
                    // remove the buddy from freelist
                    // deallocate the joint block
                    const auto buddy_blk = get_buddy(memory);

                    // return deallocate(get_joint_block(memory));
                }
            }

        }

    private:
        std::size_t get_block_count_in_lvl(int level) const
        {
            return m_block_count / (std::size_t(1) << level);
        }

        std::size_t get_block_index_in_lvl(const memory::Block & blk, int level) const
        {
            const auto block_size = Block_Size << level;
            const auto block_offset = reinterpret_cast<std::uintptr_t>(blk.base) - reinterpret_cast<std::uintptr_t>(m_memory.base);
            return block_offset / block_size;
        }

        std::size_t get_bitmap_index(const memory::Block & blk, int level) const
        {
            // Note that there is a single bit per block pair.
            // single blocks (end blocks in levels with an odd number of blocks) do not have bits.
            // blocks in the last level do no have bits either.
            // the bit represents the state of the buddy relationship, not the blocks on their own.
            // bit set means the blocks are in different allocation state (one is allocated, the other is not)
            // bit cleared means both blocks are allocated or free.

            // NOTE: This function can be optimized if all blocks have buddies.
            // NOTE: This puts a requirement on allowed (read: usable) memory sizes.

            // step 1: find the index of the block in its level.
            // NOTE: halving it gives the index of the buddy bit not counting previous level buddies.
            // step 2: find the number of buddy bits used for previous levels
            // step 3: calculate the end result

            // last level? No buddies
            if (level + 1 >= Levels)
                return -1;

            const auto index_in_lvl = get_block_index_in_lvl(blk, level);

            // check if block has buddy
            // conditions: block is the first in the buddy (otherwise, the buddy exists and is the previous block)
            // conditions: block is the last in the level (otherwise, the buddy exists and is the next block)
            // therefore the buddy cannot exist.
            if ((index_in_lvl % 2 == 0) && (index_in_lvl == get_block_count_in_lvl(level) - 1))
                return -1;

            std::size_t previous_levels_buddies = 0;
            for (int l = 0; l < level; l++)
            {
                previous_levels_buddies += get_block_count_in_lvl(l) / 2;
            }

            return previous_levels_buddies + index_in_lvl / 2;
        }

        // TODO: Lacking a better name
        // This function gets the block that is formed joining blk and it's buddy
        // memory::Block get_joint_block(const memory::Block & blk) const
        // {

        // }

        memory::Block get_buddy(const memory::Block & blk) const
        {
            // TODO:
            return {};
        }

    private:
        memory::Block m_memory, m_aux_memory;
        std::size_t m_block_count;
        Allocator & m_aux_allocator;
        BMP_Structure m_bitmap;
    };
}
