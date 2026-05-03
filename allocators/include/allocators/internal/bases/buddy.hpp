#pragma once

#include <allocators/allocator.hpp>
// #include <allocators/internal/structures/buddy_freelist_array.hpp>
#include <allocators/internal/structures/free_list.hpp>
#include <allocators/internal/structures/bitmap.hpp>

#include <limits>
#include <tuple>
#include <array>
#include <bit>

namespace dd99::memory::block_allocator::internal::base
{
    struct Buddy_Block_Address
    {
        unsigned level;
        std::size_t index;
    };

    template <std::size_t BLOCK_SIZE,
              unsigned LEVELS,
              class Bitmap_Block_Type = std::byte>
    class Buddy_Base
        // : dd99::memory::structure::detail::Buddy_Freelist_Array_Base<BLOCK_SIZE, LEVELS>
    {
    protected:
        // using Freelist_Array_Base = dd99::memory::structure::detail::Buddy_Freelist_Array_Base<BLOCK_SIZE, LEVELS>;
        using BMP = dd99::memory::structure::Bitmap<Bitmap_Block_Type>;
        using Block_Address = Buddy_Block_Address;

        // synthesize type for tuple of freelists
        using Freelist_Tuple = decltype(
            []<std::size_t ... Is>(std::index_sequence<Is...>)
            {
                return std::tuple<dd99::memory::structure::Freelist_Double_Link<(BLOCK_SIZE << Is)>...>{};
            } (std::make_index_sequence<LEVELS>())
        );

    public: // constant definitions and compile-time checks
        static constexpr unsigned Levels = LEVELS;
        static constexpr std::size_t Block_Size = BLOCK_SIZE;
        static constexpr std::size_t Max_Block_Size = Block_Size << (Levels - 1);

        static_assert(Levels != 0, "Cannot create a buddy with 0 levels");
        static_assert(std::bit_width(Block_Size) + Levels <  std::numeric_limits<std::size_t>::digits, "Block size in the last level is too high. Not supported in this architecture.");
        static_assert(Block_Size != 0, "Block size cannot be 0");

    protected: // constant definitions
        static constexpr unsigned Last_Level = Levels - 1;

    public: // static functions
        // input: block count in the first level, level to calculate
        static constexpr
        std::size_t
        calculate_block_count_in_lvl(std::size_t block_count, unsigned level)
        {
            return block_count >> level;
        }

        static constexpr
        std::size_t
        calculate_block_size_in_lvl(unsigned level)
        {
            return Block_Size << level;
        }

        // input: block count in the first level
        static constexpr
        std::size_t
        calculate_buddy_bit_count(std::size_t block_count)
        {
            std::size_t bit_count = 0;
            for (unsigned lvl = 1; lvl < Levels; ++lvl)
            {
                bit_count += calculate_block_count_in_lvl(block_count, lvl);
            }
            return bit_count;
        }

    public: // constructors and assignment
        Buddy_Base(const memory::Block & memory, std::size_t block_count, std::byte * bitmap_base)
            // : Freelist_Array_Base()
            : m_block_count(block_count)
            , m_memory(memory)
            , m_bitmap(calculate_buddy_bit_count(m_block_count), bitmap_base)
        {
            // NOTE: Cannot call deallocate here because the base does not provide full functionality.
            // NOTE: Derived class may need to do something on deallocation.
            // deallocate_all();
        }

        // Buddy_Base(const Buddy_Base &) = delete;
        // Buddy_Base & operator=(const Buddy_Base &) = delete;

        // Buddy_Base(Buddy_Base &&) = default;
        // Buddy_Base & operator=(Buddy_Base &&) = default;

    protected: // auxiliary functions

        // TODO: substitute this virtual with parameter on functions that use it
        // TODO: make those functions static and chain-up on functions that use those
        constexpr virtual
        std::byte *
        get_blocks_base() const = 0;



        template <class Callable>
        constexpr decltype(auto) visit_freelist_by_index(std::size_t index, Callable && callable)
        {
            // // Helper to get the return type when applying callable to element I
            // template <std::size_t I> using RetTypeAt = decltype(callable(std::get<I>(m_freelists)));

            // // Compute a common return type; if all are same, it's that type.
            // // If different, common_type_t will be ill‑formed → compile error.
            // template <std::size_t... Is>
            // static auto common_ret(std::index_sequence<Is...>)
            //     -> std::common_type_t<RetTypeAt<Is>...>;
            // using RetType = decltype(common_ret(std::make_index_sequence<N>{}));

            using RetType = decltype([]<std::size_t ... Is>(std::index_sequence<Is ...>)
            -> std::common_type_t<decltype(callable(std::get<Is>(std::declval<Freelist_Tuple &>())))...>
            {
                return {};
            }
            (std::make_index_sequence<Levels>()));

            // TODO: deduce return type from callable
            using FnPtr = RetType (*) (Freelist_Tuple &, Callable &&);
            constexpr auto table = []<std::size_t ... Is>(std::index_sequence<Is ...>){
                return std::array<FnPtr, Levels>{
                    [](Freelist_Tuple & freelists, Callable && callable) -> RetType
                    {
                        return callable(std::get<Is>(freelists));
                    } ...
                };
            }(std::make_index_sequence<Levels>());
            
            return table[index](m_freelists, std::forward<Callable>(callable));
        }



        // for arbitrary block sizes
        static constexpr
        unsigned
        calculate_block_level(std::size_t block_size)
        {
            // assumed block_size > 0
            const auto sub_block_count = (block_size - 1) / Block_Size + 1;
            const auto level = unsigned(std::bit_width(sub_block_count) - 1);
            return level;
        }

        // for block sizes exactly corresponding to some level
        static constexpr
        unsigned
        get_block_level(std::size_t block_size)
        {
            // TODO: Test this
            // TODO: Test performance of this
            return unsigned(std::bit_width(block_size) - std::bit_width(Block_Size));
        }


        std::size_t
        get_block_count_in_lvl(unsigned level) const
        {
            return calculate_block_count_in_lvl(m_block_count, level);
        }

        std::size_t
        get_block_index(std::byte * block_base, unsigned level) const
        {
            // assumed continuous block array
            const auto block_size = calculate_block_size_in_lvl(level);
            const auto block_offset = std::size_t(block_base - get_blocks_base());
            return block_offset / block_size;
        }

        Block_Address
        get_block_address(std::byte * block_base, unsigned level) const
        {
            const auto index = get_block_index(block_base, level);
            return {.level = level, .index = index};
        }

        Block_Address
        get_block_address(memory::Block block) const
        {
            // assumed block belongs to the allocator
            const auto level = get_block_level(block.size);
            return get_block_address(block.base, level);
        }

        Block_Address
        get_joint_block_address(Block_Address sub_block_address) const
        {
            ++sub_block_address.level;
            sub_block_address.index /= 2;
            return sub_block_address;
        }

        // does not check block exists
        std::size_t
        get_buddy_block_index(std::size_t block_index) const
        {
            return block_index ^ 1;
        }

        std::size_t
        get_buddy_bitmap_index_from_joint(Block_Address joint_block_address) const
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
            for (unsigned lvl = 1; lvl < joint_block_address.level; ++lvl)
            {
                previous_levels_joints += get_block_count_in_lvl(lvl);
            }

            return previous_levels_joints + joint_block_address.index;
        }

        memory::Block
        get_block(Block_Address address) const
        {
            const auto block_size = calculate_block_size_in_lvl(address.level);
            const auto block_offset = address.index * block_size;
            const auto block_base = get_blocks_base() + block_offset;
            return {.base = block_base, .size = block_size};
        }

        bool
        is_address_index_valid(Block_Address address) const
        {
            if (address.index < get_block_count_in_lvl(address.level))
                return true;
            return false;
        }

        bool
        block_has_buddy(Block_Address block_address) const
        {
            if (block_address.level >= Last_Level)
                return false;
            // NOTE: assumed block_address is valid
            if (block_address.index & 1)
                return true;

            const auto buddy_block_index = get_buddy_block_index(block_address.index);
            return is_address_index_valid({.level = block_address.level, .index = buddy_block_index});
        }

        void deallocate(const memory::Block memory, Block_Address address)
        {
            if (block_has_buddy(address))
            {
                const auto joint_block_address = get_joint_block_address(address);
                const auto bmp_index = get_buddy_bitmap_index_from_joint(joint_block_address);
                const auto bmp_value = m_bitmap.toggle(bmp_index);
                if (!bmp_value)
                {   // buddy is free. join the blocks
                    const auto buddy_block_index = get_buddy_block_index(address.index);
                    const auto buddy_block = get_block({.level = address.level, .index = buddy_block_index});

                    visit_freelist_by_index(address.level, [buddy_block](auto & freelist){ freelist.remove(buddy_block); });

                    // [&]<std::size_t ... Is>(std::index_sequence<Is ...>) {
                    //     switch (address.level)
                    //     {
                    //         ((case Is: m_freelists[Is].remove(buddy_block); break;) ... );
                    //     }
                    // }(std::make_index_sequence<LEVELS>());

                    // Freelist_Array_Base::m_freelists[address.level].remove(buddy_block);

                    const auto joint_block = get_block(joint_block_address);
                    return deallocate(joint_block, joint_block_address);
                }
            }

            visit_freelist_by_index(address.level, [memory](auto & freelist){ freelist.push(memory); });

            // Freelist_Array_Base::m_freelists[address.level].push(memory);
        }

    public: // specialized functions
        [[nodiscard]]
        memory::Block
        allocate_from_level(unsigned level)
        {
            // allocation too large?
            if (level >= Levels)
                return {};

            // step 1: get the block
            // try to get a block from the freelist for the current level.
            // otherwise, allocate a block from the next level and split it.
            // the first half is put in the freelist of the current level.
            // the other half is what we use.

            Block blk;

            // if (Freelist_Array_Base::m_freelists[level].empty())
            if (visit_freelist_by_index(level, [](auto & freelist){ return freelist.empty(); }))
            {
                blk = allocate_from_level(level + 1);
                if (!blk) return {};

                blk.size /= 2;
                // Freelist_Array_Base::m_freelists[level].push(blk);
                visit_freelist_by_index(level, [blk](auto & freelist){ freelist.push(blk); });

                blk.base = blk.get_end();

                // NOTE: We know blk has a buddy
                // NOTE: Could optimize based on that
            }
            else
            {
                // blk = Freelist_Array_Base::m_freelists[level].pop();
                blk = visit_freelist_by_index(level, [](auto & freelist){ return freelist.pop(); });
            }

            // step 2: got a block, toggle the buddy bit.
            // some blocks do not have a buddy.

            const auto block_address = get_block_address(blk.base, level);
            if (block_has_buddy(block_address))
            {
                const auto joint_block_address = get_joint_block_address(block_address);
                const auto bmp_index = get_buddy_bitmap_index_from_joint(joint_block_address);
                m_bitmap.toggle(bmp_index);
            }

            return blk;
        }

    public: // allocator interface implementations
        [[nodiscard]]
        memory::Block
        allocate(std::size_t requested_size)
        {
            if (requested_size == 0)
                return {};

            const auto required_block_level = calculate_block_level(requested_size);
            return allocate_from_level(required_block_level);
        }

        void
        deallocate(const memory::Block & memory)
        {
            // NOTE: it is assumed the memory block was not modified.
            // NOTE: the block size is assumed to be exactly the size of blocks from some level.

            if (!owns(memory))
                return;

            const auto address = get_block_address(memory);

            deallocate(memory, address);
        }

        void
        deallocate_all()
        {
            m_bitmap.reset();

            for (unsigned lvl = 0; lvl < Levels; ++lvl)
            {
                // Freelist_Array_Base::m_freelists[lvl].clear();
                visit_freelist_by_index(lvl, [](auto & freelist){ freelist.clear(); });
            }

            // Build freelists:
            // - Add all blocks on the last level to freelist
            // - Add blocks without buddies to freelist

            const auto last_level_block_count = calculate_block_count_in_lvl(m_block_count, Last_Level);
            for (std::size_t index = 0; index < last_level_block_count; ++index)
            {
                const auto current_block = get_block({.level = Last_Level, .index = index});
                // Freelist_Array_Base::m_freelists[Last_Level].push(current_block);
                std::get<Last_Level>(m_freelists).push(current_block);
            }

            for (unsigned lvl = 0; lvl < Last_Level; ++lvl)
            {
                const auto block_count = calculate_block_count_in_lvl(m_block_count, lvl);
                if (block_count == 0) break; // upper levels may be empty. this prevents wrap-around on `block_count - 1` on the next line.
                const Block_Address last_block_address{.level = lvl, .index = block_count - 1};
                if (!block_has_buddy(last_block_address))
                {
                    const auto last_block = get_block(last_block_address);
                    // Freelist_Array_Base::m_freelists[lvl].push(last_block);
                    visit_freelist_by_index(lvl, [last_block](auto & freelist){ freelist.push(last_block); });
                }
            }
        }

        bool
        owns(std::byte * memory) const
        {
            return m_memory.contains(memory);
        }

        bool
        owns(const memory::Block & memory) const
        {
            return m_memory.contains(memory);
        }

    protected: // members
        std::size_t m_block_count;
        memory::Block m_memory;
        BMP m_bitmap;
        Freelist_Tuple m_freelists;
        
    };

    static_assert(Block_Allocator<Buddy_Base<2*sizeof(void*), 1>>, "This definition doesn't comply with the `Block_Allocator` concept");

}
