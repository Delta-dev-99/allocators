#pragma once

#include <allocators/library_configuration/cpp_config.hpp>
#include <allocators/structures/blocks/memory_block.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_block_address.hpp>
#include <allocators/alignment.hpp>
#include <array>
#include <bit>
#include <concepts>



namespace dd99::memory::block_allocator::buddy_namespace
{

    // A type that describes the layout of the managed memory
    // does not know or care about bookkeeping data structures
    // TODO: ensure the interface is now complete
    template <class T>
    concept Layout_Concept = requires
    {
        typename T::block_address_type;
    } && requires(T & layout, std::byte * memory_base, memory::block blk, T::block_address_type blk_address)
    {
        typename T::level_type;
        typename T::index_type;
        typename T::block_address_type;

        { T::levels } -> std::same_as<const typename T::level_type &>;
        { T::block_size } -> std::same_as<const std::size_t &>;


        { T::get_block_address(memory_base, blk) } -> std::same_as<typename T::block_address_type>;
        { T::get_block(memory_base, blk_address) } -> std::same_as<block>;

        { T::get_joint_block_address(blk_address) } -> std::same_as<typename T::block_address_type>;
        { T::get_buddy_block_address(blk_address) } -> std::same_as<typename T::block_address_type>;

        { T::get_block_level(blk.size) } -> std::same_as<typename T::level_type>;
        { T::get_level_block_size(blk_address.level) } -> std::same_as<typename std::size_t>;

        { layout.get_block_address(blk) } -> std::same_as<typename T::block_address_type>;
        { layout.get_block_address(blk.base, blk_address.level) } -> std::same_as<typename T::block_address_type>;
        { layout.get_block(blk_address) } -> std::same_as<block>;

        { layout.get_total_joint_block_count() } -> std::same_as<std::size_t>;
        { layout.get_cumulative_joint_block_count(blk_address.level) } -> std::same_as<std::size_t>;
        { layout.get_level_block_count(blk_address.level) } -> std::same_as<typename T::index_type>;
        
        { layout.block_has_buddy(blk_address) } -> std::same_as<bool>;
        { layout.is_index_valid(blk_address) } -> std::same_as<bool>;
    };


    // The standard buddy layout.
    // This describes the managed memory as a contiguous array of blocks
    // 
    // *** Template arguments:
    // `Managed_Memory_Block_Type` is the type used for the block of memory to be managed by the allocator.
    // the user can pass a normal block, an auto-freeing block, or a custom type compliant with the interface.
    // 
    // 
    template <class                             Block_Address_Type,
              std::size_t                       Block_Size,
              Block_Address_Type::level_type    Levels,
              std::size_t                       Last_Level_Alignment = Block_Size << (Levels-1),
              class                             Managed_Memory_Block_Type = block>
    struct buddy_standard_layout
    {
        // static_assert(Layout_Concept<buddy_standard_layout>);

        using managed_memory_block_type = Managed_Memory_Block_Type;
        using block_address_type = Block_Address_Type;
        using level_type = block_address_type::level_type;
        using index_type = block_address_type::index_type;
        
        static constexpr auto levels = Levels;
        static constexpr auto last_level = levels - 1;
        static constexpr auto block_size = Block_Size;
        static constexpr auto last_level_alignment = Last_Level_Alignment;
        static constexpr auto first_level_alignment = last_level_alignment >> (levels - 1);

        static_assert(last_level_alignment <= (block_size << last_level),
            "alignment larger than block size is impossible");

        // *** auxiliary functions ***
        // ###########################

        static constexpr
        std::size_t
        get_level_alignment(level_type level)
        {
            return std::max(std::size_t{1}, last_level_alignment >> (last_level - level));
        }

        static constexpr
        level_type
        get_alignment_level(std::size_t alignment)
        {
            DD99_ALLOCATORS_ASSERT_HARDENED("alignment too big", alignment <= last_level_alignment);
            DD99_ALLOCATORS_ASSERT_DEBUG("alignment must be a power of 2", std::has_single_bit(alignment));
            // NOTE: `alignment > last_level_alignment` results in `last_level + 1` which is correctly handled on allocation, but the returned block is not aligned as expected
            // NOTE: `alignment == 0` causes a division by zero, but this is a user error. minimum alignment is 1, which is effectively unaligned.
            if (alignment <= first_level_alignment) return 0;
            else return levels - std::bit_width(last_level_alignment / alignment);
        }

        static constexpr
        std::size_t
        get_level_block_size(level_type level)
        {
            return block_size << level;
        }

        static constexpr
        index_type
        get_level_block_count(index_type base_block_count, level_type level)
        {
            return base_block_count >> level;
        }

        static constexpr
        index_type
        get_level_block_count(block memory, level_type level)
        {
            return get_level_block_count(memory.size / block_size, level);
        }

        // this also gives the number of bits needed for buddy block state
        static constexpr
        std::size_t
        get_total_joint_block_count(index_type base_block_count)
        {
            std::size_t count = 0;
            for (level_type level = 1; level < levels; ++level)
            {
                count += get_level_block_count(base_block_count, level);
            }
            return count;
        }

        static constexpr
        std::size_t
        get_total_block_count(index_type base_block_count)
        {
            return base_block_count + get_total_joint_block_count(base_block_count);
        }


        // for arbitrary block sizes
        static constexpr
        unsigned
        calculate_block_level(std::size_t blk_size)
        {
            // assumed block_size > 0
            const auto sub_block_count = (blk_size - 1) / block_size + 1;
            const auto level = static_cast<level_type>(std::bit_width(sub_block_count) - 1);
            return level;
        }

        // for block sizes exactly corresponding to some level
        static constexpr
        level_type
        get_block_level(std::size_t blk_size)
        {
            // TODO: Test this
            // TODO: Test performance of this
            return static_cast<level_type>(std::bit_width(blk_size) - std::bit_width(block_size));
        }

        static constexpr
        index_type
        get_block_index(std::byte * memory_base, std::byte * block_base, level_type level)
        {
            // continuous block array
            const auto blk_size = get_level_block_size(level);
            const auto block_offset = static_cast<std::size_t>(block_base - memory_base);
            // TODO: this division could be optimized when block sizes are powers of 2
            // TODO: can we use assertions to guide optimization assumptions? maybe instead of disabling assertions on release, we could change it to a compiler assumption directive.
            // TODO: we could then assert(std::has_single_bit(blk_size))
            // TODO: an alternative would be to use compile-time branching, which would also allow non-power-of-2 block sizes
            return block_offset / blk_size;
        }


        static constexpr
        block_address_type
        get_block_address(std::byte * memory_base, std::byte * block_base, level_type level)
        {
            const auto index = get_block_index(memory_base, block_base, level);
            return {.level = level, .index = index};
        }


        // *** functions that may become interface ***
        // ###########################################

        static constexpr
        block_address_type
        get_joint_block_address(block_address_type sub_blk_address)
        {
            return {.level = sub_blk_address.level + 1, .index = sub_blk_address.index / 2};
        }

        // does not check block exists
        static constexpr
        index_type
        get_buddy_block_index(index_type blk_index)
        {
            return blk_index ^ 1;
        }

        // does not check block exists
        static constexpr
        block_address_type
        get_buddy_block_address(block_address_type blk_addr)
        { return {.level = blk_addr.level, .index = get_buddy_block_index(blk_addr.index)}; }


        // *** Interface functions ***
        // ###########################

        static constexpr
        block_address_type
        get_block_address(std::byte * memory_base, block blk)
        {
            // assumes blk belongs to the allocator
            const auto level = get_block_level(blk.size);
            return get_block_address(memory_base, blk.base, level);
        }

        static constexpr
        block
        get_block(std::byte * memory_base, block_address_type blk_address)
        {
            const auto blk_size = get_level_block_size(blk_address.level);
            const auto blk_offset = blk_address.index * blk_size; // TODO: check optimization for blk_size power of 2
            const auto blk_base = memory_base + blk_offset;
            return {.base = blk_base, .size = blk_size};
        }



        // *** instance functions ***
        // ##########################

        constexpr
        buddy_standard_layout(managed_memory_block_type mem_blk)
            : m_memory{std::move(mem_blk)}
            , m_block_count{static_cast<index_type>(m_memory.size / block_size)}
        {
            DD99_ALLOCATORS_ASSERT_HARDENED("managed memory base must be appropriately aligned", is_aligned(m_memory.get_base(), last_level_alignment));
            DD99_ALLOCATORS_ASSERT_HARDENED("managed memory too big for the chosen addressing type", static_cast<std::size_t>(m_block_count) == (m_memory.size / block_size));

            // calculate cumulative joint block counts
            m_cumulative_joint_block_count[0] = 0;
            std::size_t count = 0;
            for (level_type level = 1; level < levels; ++level)
            {
                count += get_level_block_count(level);
                m_cumulative_joint_block_count[level] = count;
            }
        }


        [[nodiscard]]
        constexpr
        index_type
        get_level_block_count(level_type level) const
        { return get_level_block_count(m_block_count, level); }

        [[nodiscard]]
        constexpr
        std::size_t
        get_cumulative_joint_block_count(level_type level) const
        { return m_cumulative_joint_block_count[level]; }

        [[nodiscard]]
        constexpr
        std::size_t
        get_total_joint_block_count() const
        { return m_cumulative_joint_block_count[last_level]; }

        
        // this overload avoids calculating the level when the user can provide it
        [[nodiscard]]
        constexpr
        block_address_type
        get_block_address(std::byte * block_base, level_type level) const
        { return get_block_address(m_memory.base, block_base, level); }

        [[nodiscard]]
        constexpr
        block_address_type
        get_block_address(block blk) const
        { return get_block_address(m_memory.base, blk); }

        [[nodiscard]]
        constexpr
        block
        get_block(block_address_type blk_address) const
        { return get_block(m_memory.base, blk_address); }


        [[nodiscard]]
        constexpr
        bool
        is_index_valid(block_address_type blk_addr) const
        {
            // NOTE: assumed level is valid
            return blk_addr.index < get_level_block_count(blk_addr.level);
        }

        [[nodiscard]]
        constexpr
        bool
        block_has_buddy(block_address_type blk_addr) const
        {
            if (blk_addr.level >= last_level) return false; // blocks on the last level don't have buddies
            // NOTE: assumed block_address is valid
            if (blk_addr.index & 1) return true;

            const auto buddy_addr = get_buddy_block_address(blk_addr);
            return is_index_valid(buddy_addr);
        }


        managed_memory_block_type m_memory;
        index_type m_block_count; // TODO: should we just calculate this on-the-fly when needed?
        std::array<std::size_t, levels> m_cumulative_joint_block_count; // index n gives total joint blocks at levels 0 through n inclusive. last index gives total joint block count. joint blocks are blocks from levels > 0, so index 0 gives count 0.
    };

}
