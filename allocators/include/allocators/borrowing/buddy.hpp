#pragma once

#include <allocators/basic/allocator.hpp>
#include <allocators/structures/bitmap.hpp>
#include <allocators/structures/free_list.hpp>


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
            memory::structure::Freelist_Double_Link m_freelists[Levels];

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

        // used to address a block in the buddy system
        struct Block_Address
        {
            int level;
            std::size_t index;
        };

    public: // static consts
        static constexpr int Levels = LEVELS;
        static constexpr std::size_t Block_Size = BLOCK_SIZE;
        static constexpr std::size_t Max_Block_Size = Block_Size << (Levels - 1);
        
        static_assert(Levels > 0); // NOTE: For lvl = 1 there is no point.
        static_assert(Levels < std::numeric_limits<std::size_t>::digits); // Just in case...
        static_assert(Block_Size > 0);

    public: // statics
        static constexpr std::size_t calculate_block_count_in_lvl(std::size_t memory_size, int level)
        {
            const auto block_count = memory_size / Block_Size;
            return block_count / (std::size_t(1) << level);
        }

        static constexpr std::size_t calculate_bmp_bit_count(std::size_t memory_size)
        {
            std::size_t bit_count = 0;
            for (int lvl = 1; lvl < Levels; lvl++)
            {
                bit_count += calculate_block_count_in_lvl(memory_size, lvl);
            }
            return bit_count;
        }

        static constexpr std::size_t calculate_aux_allocation(std::size_t memory_size)
        {
            const auto bit_count = calculate_bmp_bit_count(memory_size);
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
            , m_block_count(memory.size / Block_Size)
            , m_aux_allocator(aux_allocator)
            , m_memory(memory)
            , m_aux_memory(m_aux_allocator.allocate(calculate_aux_allocation(memory.size)))
            , m_bitmap(calculate_bmp_bit_count(memory.size), m_aux_memory.base)
        {
            // NOTE: This is safe because the bitmap structure does not operate on the memory during construction
            if (!m_aux_memory)
                throw std::runtime_error{"Buddy Allocator: Borrowed allocator initialization: Auxiliary allocation failed"};

            deallocate_all();
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
            // otherwise, allocate a block from the next level and split it.
            // the first half is put in the freelist of the current level.
            // the other half is what we use.

            Block blk;

            if (Freelist_Base::m_freelists[requested_level].empty())
            {
                blk = allocate_from_level(requested_level + 1);
                if (!blk) return {};

                blk.size /= 2;
                Freelist_Base::m_freelists[requested_level].push(blk);

                blk.base = blk.get_end();

                // NOTE: We know blk has a buddy
                // NOTE: Could optimize based on that
            }
            else
                blk = Freelist_Base::m_freelists[requested_level].pop();

            // step 2: got a block, toggle the buddy bit.
            // some blocks do not have a buddy.

            const auto block_address = get_block_address(blk, requested_level);
            if (block_has_buddy(block_address))
            {
                const auto joint_block_address = get_joint_block_address(block_address);
                const auto bmp_index = get_bitmap_index(joint_block_address);
                m_bitmap.toggle(bmp_index);
            }

            return blk;
        }

        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size)
        {
            // Find out what level the allocation requires
            // relay to the function that allocates from levels

            if (requested_size == 0)
                return {};

            const int required_block_level = get_block_level(requested_size);
            return allocate_from_level(required_block_level);
        }

        void deallocate(const memory::Block & memory)
        {
            // NOTE: it is assumed the memory block was not modified.
            // NOTE: the block size is assumed to be exactly the size of blocks from some level.

            if (!owns(memory))
                return;

            const auto address = get_block_address(memory);

            deallocate(memory, address);
        }

        void deallocate_all()
        {
            m_bitmap.reset();

            for (int l = 0; l < Levels; l++)
            {
                Freelist_Base::m_freelists[l].clear();
            }

            // Build freelists
            // step 1: Add all blocks on the last level to freelist
            // step 2: Add blocks without buddies to freelist

            for (Block_Address address{.level = Levels - 1, .index = 0}; address.index < get_block_count_in_lvl(Levels - 1); address.index++)
            {
                Freelist_Base::m_freelists[Levels - 1].push(get_block(address));
            }

            for (int lvl = 0; lvl < Levels - 1; lvl++)
            {
                const auto block_count = get_block_count_in_lvl(lvl);
                const Block_Address last_block_addr{.level = lvl, .index = block_count - 1};
                if (!block_has_buddy(last_block_addr))
                    Freelist_Base::m_freelists[lvl].push(get_block(last_block_addr));
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

    private:
        static constexpr std::size_t get_block_size_in_lvl(int level)
        {
            return Block_Size << level;
        }

        static constexpr int get_block_level(std::size_t requested_size)
        {
            // assumed requested_size > 0
            const auto block_count = (requested_size - 1) / Block_Size + 1;
            const int level = std::bit_width(block_count) - 1;
            return level;
        }

        static constexpr int get_block_level(const memory::Block & blk)
        {
            // assumed blk is a block from this allocator type
            const auto sub_block_count = blk.size / Block_Size;
            const int level = std::bit_width(sub_block_count) - 1;
            return level;
        }

        std::size_t get_block_count_in_lvl(int level) const
        {
            return m_block_count / (std::size_t(1) << level);
        }

        std::size_t get_block_index_in_lvl(const memory::Block & blk, int level) const
        {
            const auto block_size = get_block_size_in_lvl(level);
            const auto block_offset = reinterpret_cast<std::uintptr_t>(blk.base) - reinterpret_cast<std::uintptr_t>(m_memory.base);
            return block_offset / block_size;
        }

        Block_Address get_block_address(const memory::Block & blk, int level) const
        {
            const auto index = get_block_index_in_lvl(blk, level);
            return Block_Address{.level = level, .index = index};
        }

        Block_Address get_block_address(const memory::Block & blk) const
        {
            const auto level = get_block_level(blk);
            return get_block_address(blk, level);
        }

        Block_Address get_joint_block_address(Block_Address sub_block_address) const
        {
            // NOTE: does not check if the block exists
            sub_block_address.level++;
            sub_block_address.index /= 2;
            return sub_block_address;
        }

        Block_Address get_buddy_block_address(Block_Address address) const
        {
            // NOTE: does not check the buddy exists.
            address.index ^= 1;
            return address;
        }

        std::size_t get_bitmap_index(Block_Address joint_blk_address) const
        {
            // BIG NOTE: The input is not one of the buddies, but the joint block they form

            // In the buddy allocation model, 2 blocks that are buddies are joined to form a bigger block (the Joint Block)
            // Note that there is a single bit per block pair (means: per Joint Block).
            // Single blocks (end blocks in levels with an odd number of blocks) do not have bits.
            // That is because single blocks do not join with others to form bigger blocks.
            // Blocks in the last level do no have bits either. They do not form bigger blocks.

            // The bit represents the state of the buddy relationship, not the blocks on their own.
            // Bit set means the blocks are in different allocation state (one is allocated, the other is not)
            // Bit cleared means both blocks are allocated or free.

            // NOTE: This function can be optimized if all blocks have buddies.
            // NOTE: This puts a requirement on allowed (read: usable) memory sizes.

            // NOTE: Halving the index gives the index of the buddy bit in this level.
            // NOTE: That index is the same as the index of the joint block in its level.

            // step 1: Find the number of buddy bits used for previous levels
            // step 2: Calculate the end result

            std::size_t previous_levels_joints = 0;
            for (int l = 1; l < joint_blk_address.level; l++)
            {
                previous_levels_joints += get_block_count_in_lvl(l);
            }

            return previous_levels_joints + joint_blk_address.index;
        }

        memory::Block get_block(Block_Address address) const
        {
            const auto block_size = get_block_size_in_lvl(address.level);
            const auto block_offset = address.index * block_size;
            const auto block_base = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(m_memory.base) + block_offset);
            return {.base = block_base, .size = block_size};
        }

        bool is_address_index_valid(Block_Address address) const
        {
            if (address.index < get_block_count_in_lvl(address.level))
                return true;
            return false;
        }

        bool block_has_buddy(Block_Address block_address) const
        {
            if (block_address.level >= Levels - 1)
                return false;
            // NOTE: assumed block_address is valid
            const auto buddy_block_address = get_buddy_block_address(block_address);
            // NOTE: if buddy index is even, it exists; no need to check.
            return (buddy_block_address.index & 1) ? is_address_index_valid(buddy_block_address) : true;
        }

        void deallocate(const memory::Block & memory, Block_Address address)
        {
            if (block_has_buddy(address))
            {
                const auto joint_block_address = get_joint_block_address(address);
                const auto bmp_index = get_bitmap_index(joint_block_address);
                const auto bmp_value = m_bitmap.toggle(bmp_index);
                if (!bmp_value)
                {   // buddy is free. join the blocks
                    const auto buddy_block_address = get_buddy_block_address(address);
                    const auto buddy_block = get_block(buddy_block_address);
                    Freelist_Base::m_freelists[address.level].remove(buddy_block);
                    return deallocate(joint_block_address);
                }
            }
            
            Freelist_Base::m_freelists[address.level].push(memory);
        }

        void deallocate(Block_Address address)
        {
            const auto block = get_block(address);
            deallocate(block, address);
        }

    private:
        std::size_t m_block_count;
        Allocator & m_aux_allocator;
        memory::Block m_memory, m_aux_memory;
        BMP_Structure m_bitmap;
    };
}
