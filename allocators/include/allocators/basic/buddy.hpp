#pragma once

#include <allocators/basic/allocator.hpp>
#include <allocators/internal/structures/bitmap.hpp>
#include <allocators/internal/structures/buddy_freelist_array.hpp>


namespace dd99::memory::block_allocator
{

    // NOTE: Max_Block_Size is an inclussive upper bound
    template <std::size_t BLOCK_SIZE = (1 << 12),
              unsigned LEVELS = 11,
              class Bitmap_Element_T = std::uint8_t>
    class Buddy : public Allocator, public dd99::memory::structure::detail::Buddy_Freelist_Array_Base<BLOCK_SIZE, LEVELS>
    {
        using Freelist_array_base = dd99::memory::structure::detail::Buddy_Freelist_Array_Base<BLOCK_SIZE, LEVELS>;
        using BMP_Structure = dd99::memory::structure::Bitmap<Bitmap_Element_T>;

        struct Block_Address
        {
            unsigned level;
            std::size_t index;
        };

    public:
        static constexpr unsigned Levels = LEVELS;
        static constexpr std::size_t Block_Size = BLOCK_SIZE;
        static constexpr std::size_t Max_Block_Size = Block_Size << (Levels - 1);

        static_assert(Levels > 0); // NOTE: For lvl = 1 there is no point.
        static_assert(Levels < std::numeric_limits<std::size_t>::digits); // Just in case...
        static_assert(Block_Size > 0);

    public: // statics
        static constexpr std::size_t calculate_block_count_in_lvl(std::size_t basic_block_count, unsigned level)
        {
            // return basic_block_count / (std::size_t(1) << level);
            return basic_block_count >> level;
        }

        static constexpr std::size_t calculate_bmp_bit_count(std::size_t basic_block_count)
        {
            std::size_t bit_count = 0;
            for (unsigned lvl = 1; lvl < Levels; lvl++)
            {
                bit_count += calculate_block_count_in_lvl(basic_block_count, lvl);
            }
            return bit_count;
        }

        static constexpr std::size_t calculate_basic_block_count(std::size_t memory_size)
        {
            if (memory_size < BMP_Structure::Block_Size + Block_Size)
                return 0;

            std::size_t block_count = memory_size / Block_Size;
            
            // Iteratively aproach block count
            while (true)
            {
                const auto bmp_bits = calculate_bmp_bit_count(block_count);
                const auto bmp_size = BMP_Structure::calculate_block_count(bmp_bits) * BMP_Structure::Block_Size;
                const auto next_block_count = (memory_size - bmp_size) / Block_Size;
                if (next_block_count - block_count <= BMP_Structure::calculate_block_count(Levels))
                    break;

                block_count = next_block_count;
            }

            return block_count;
        }

        // static constexpr double ratio(std::size_t n_blocks)
        // {
        //     return double(n_blocks * Block_Size)/(BMP_Structure::calculate_block_count(bitmap_bits(n_blocks)) * BMP_Structure::Block_Size);
        // }

    public:
        Buddy(const memory::Block &memory)
            : Freelist_array_base()
            , m_block_count(calculate_basic_block_count(memory.size))
            , m_memory(memory)
            , m_bitmap(calculate_bmp_bit_count(m_block_count), memory.base)
        {
            m_blocks_base = memory.get_end() - m_block_count * Block_Size;
            deallocate_all();
        }

    public:
        [[nodiscard]]
        memory::Block allocate_from_level(unsigned requested_level)
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

            if (Freelist_array_base::m_freelists[requested_level].empty())
            {
                blk = allocate_from_level(requested_level + 1);
                if (!blk) return {};

                blk.size /= 2;
                Freelist_array_base::m_freelists[requested_level].push(blk);

                blk.base = blk.get_end();

                // NOTE: We know blk has a buddy
                // NOTE: Could optimize based on that
            }
            else
                blk = Freelist_array_base::m_freelists[requested_level].pop();

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

            const auto required_block_level = get_block_level(requested_size);
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

            for (unsigned l = 0; l < Levels; l++)
            {
                Freelist_array_base::m_freelists[l].clear();
            }

            // Build freelists
            // step 1: Add all blocks on the last level to freelist
            // step 2: Add blocks without buddies to freelist

            for (Block_Address address{.level = Levels - 1, .index = 0}; address.index < get_block_count_in_lvl(Levels - 1); address.index++)
            {
                Freelist_array_base::m_freelists[Levels - 1].push(get_block(address));
            }

            for (unsigned lvl = 0; lvl < Levels - 1; lvl++)
            {
                const auto block_count = get_block_count_in_lvl(lvl);
                const Block_Address last_block_addr{.level = lvl, .index = block_count - 1};
                if (!block_has_buddy(last_block_addr))
                    Freelist_array_base::m_freelists[lvl].push(get_block(last_block_addr));
            }
        }

        bool owns(std::byte *memory) const
        {
            return m_memory.contains(memory);
        }

        bool owns(const memory::Block& memory) const
        {
            return m_memory.contains(memory);
        }

    private:
        static constexpr std::size_t get_block_size_in_lvl(unsigned level)
        {
            return Block_Size << level;
        }

        static constexpr unsigned get_block_level(std::size_t requested_size)
        {
            // assumed requested_size > 0
            const auto block_count = (requested_size - 1) / Block_Size + 1;
            const auto level = unsigned(std::bit_width(block_count) - 1);
            return level;
        }

        static constexpr unsigned get_block_level(const memory::Block & blk)
        {
            // assumed blk is a block from this allocator type
            const auto sub_block_count = blk.size / Block_Size;
            const auto level = unsigned(std::bit_width(sub_block_count) - 1);
            return level;
        }

        std::size_t get_block_count_in_lvl(unsigned level) const
        {
            return m_block_count / (std::size_t(1) << level);
        }

        std::size_t get_block_index_in_lvl(const memory::Block & blk, unsigned level) const
        {
            const auto block_size = get_block_size_in_lvl(level);
            const auto block_offset = blk.base - m_blocks_base;
            return block_offset / block_size;
        }

        Block_Address get_block_address(const memory::Block & blk, unsigned level) const
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
            for (unsigned l = 1; l < joint_blk_address.level; l++)
            {
                previous_levels_joints += get_block_count_in_lvl(l);
            }

            return previous_levels_joints + joint_blk_address.index;
        }

        memory::Block get_block(Block_Address address) const
        {
            const auto block_size = get_block_size_in_lvl(address.level);
            const auto block_offset = address.index * block_size;
            const auto block_base = m_blocks_base + block_offset;
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
                    Freelist_array_base::m_freelists[address.level].remove(buddy_block);
                    return deallocate(joint_block_address);
                }
            }
            
            Freelist_array_base::m_freelists[address.level].push(memory);
        }

        void deallocate(Block_Address address)
        {
            const auto block = get_block(address);
            deallocate(block, address);
        }

    private:
        std::size_t m_block_count;
        memory::Block m_memory;
        std::byte * m_blocks_base;
        BMP_Structure m_bitmap;
        // typename detail::freelist_collection<Block_Size, Levels>::type m_freelists;
    };
}


// NOTES:

// TODO:
// Add calculation for optimum location of bitmap and blocks
// Where to leave unused space? Between bitmap and blocks? At the ends? Both? Something else?
// Take into account alignment for blocks (blocks may be of not-power-of-2 sizes)
