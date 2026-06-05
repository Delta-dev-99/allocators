#pragma once

#include <allocators/block_allocators/basic/buddy/buddy_layout.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_state.hpp>
#include <allocators/structures/linked_list.hpp>
#include <allocators/structures/bitmap.hpp>
#include <array>



namespace dd99::memory::block_allocator::buddy_namespace
{

    // The intrusive state implementation stores linked list nodes directly on the managed free blocks, which helps reduce the memory footprint.
    // An external XOR bitmap is used to store buddy state. The user must provide the memory block for this bitmap.
    // 
    // The `LEVELS` template parameter:
    // This parameter determines the number of linked lists to create and the number of buddy states to manage for a given number of lowest-level blocks.
    // 
    // The `Block_Address_Type` template parameter:
    // The type used to represent the address of a block in the buddy system. Can be customized to balance required bookkeeping memory vs representable memory range for a given Block_Size used in the buddy allocator. This type is expected to encode both the level and index of the block within the buddy system's hierarchy. The default type is `dd99::memory::block_allocator::buddy_policy::block_address<>`, which uses unsigned integers for both level and index. This type is used by the policy to identify blocks when performing operations such as pushing, popping, and merging blocks in the buddy system. The user can provide a custom type that satisfies the expected interface if they want to use a different encoding for block addresses.
    // 
    // The `State_Block_Type` template parameter:
    // The type of the memory block provided by the user to store the buddy state and free list information. Intended to be deduced via factory function. This block is expected to be large enough to hold the necessary data structures for managing the buddy system's state across all levels. The policy will use this block to maintain the state of which blocks are free and which are allocated, as well as to manage the linked lists of free blocks at each level. The user is responsible for providing a suitable memory block for this purpose, and the policy will handle the organization and management of this block internally.
    // This is part of the lifetime management customization mechanism used in this library. Allows either using a plain memory block, or a RAII auto-freeing memory block type.
    template <Layout_Concept Layout, class State_Memory_Block_Type, class Bitmap_Element_Type = std::byte>
    struct buddy_intrusive_state
    {
        // Check whether this policy type actually satisfies the requirements for buddy policy types.
        // This is just a cost-free sanity check which should always pass.
        // Failure here likely means that either a template parameter was terribly wrong, or that there's an error in this implementation.
        static_assert(State_Concept<buddy_intrusive_state>,
            "buddy_intrusive_state does not satisfy State_Concept requirements.");

        using layout_type = Layout;
        using state_memory_block_type = State_Memory_Block_Type;

        using block_address_type = layout_type::block_address_type;
        using level_type = block_address_type::level_type;
        using index_type = block_address_type::index_type;

        static constexpr auto levels = layout_type::levels;
        static constexpr auto block_size = layout_type::block_size;

        static constexpr auto last_level = levels - 1;

        using freelist_type = dd99::memory::structure::basic_linked_list;
        using bitmap_type = dd99::memory::structure::Bitmap<Bitmap_Element_Type>;

        static_assert(sizeof(freelist_type::node) <= block_size,
            "block_size is too small to hold a freelist node.");

        // TODO: assert that freelist node alignment requirements are met by all blocks
        // NOTE: this means that alignment must be a divisor of block size
        // NOTE: and managed memory base (inside layout) must have this alignment.

        // a type for tracking the state of buddy blocks
        // wraps a bitmap and adds a convenient interface for the buddy allocator
        struct buddy_state_tracker
        {
            [[nodiscard]]
            constexpr
            std::size_t
            get_bitmap_index_from_joint(block_address_type joint_blk_address, const layout_type & layout) const
            {
                // TODO: assert(joint_blk_address.level > 0). Blocks at level 0 aren't joint blocks.
                return joint_blk_address.index + layout.get_cumulative_joint_block_count(joint_blk_address.level - 1);
            }

            constexpr
            bool
            toggle_joint_block_state(block_address_type joint_blk_address, const layout_type & layout)
            {
                auto bitmap_index = get_bitmap_index_from_joint(joint_blk_address, layout);
                return m_bitmap.toggle(bitmap_index);
            }

            constexpr void reset() { m_bitmap.reset(); }


            bitmap_type m_bitmap;
        };



        constexpr
        buddy_intrusive_state(layout_type && layout, state_memory_block_type && state_memory)
            : m_layout{std::forward<layout_type>(layout)}
            , m_state_memory{std::forward<state_memory_block_type>(state_memory)}
            , m_state_tracker{bitmap_type{m_layout.get_total_joint_block_count(), m_state_memory.base}}
        {
            // TODO: assert state memory is enough
            // assert(m_state_memory.size >= bitmap_type::calculate_block_count(m_layout.get_total_joint_block_count()) * bitmap_type::Block_Size)
            // TODO: assert state memory is properly aligned
            // assert(state_memory.base & (bitmap_type::Block_Alignment - 1) == 0)
            init_freelists();
        }


        constexpr
        void
        init_freelists()
        {
            // Build freelists:
            // - Add all blocks on the last level to freelist
            // - Add blocks without buddies to freelist

            const auto last_level_block_count = m_layout.get_level_block_count(last_level);
            for (index_type blk_index = 0; blk_index < last_level_block_count; ++blk_index)
            {
                const auto current_block = m_layout.get_block({.level = last_level, .index = blk_index});
                m_freelist_collection[last_level].push(current_block.base);
            }

            for (level_type level = 0; level < last_level; ++level)
            {
                const auto block_count = m_layout.get_level_block_count(level);
                if (block_count == 0) break; // upper levels may be empty. this prevents integer wrap-around on `block_count - 1` on the next line.
                const block_address_type last_block_address{.level = level, .index = block_count - 1};
                if (!m_layout.block_has_buddy(last_block_address))
                {
                    const auto last_block = m_layout.get_block(last_block_address);
                    m_freelist_collection[level].push(last_block.base);
                }
            }
        }

        constexpr
        void
        reset()
        {
            for (auto & x : m_freelist_collection) x.clear();
            m_state_tracker.reset();

            init_freelists();
        }


        constexpr
        void
        push(level_type level, std::byte * block_base)
        {
            auto block_address = m_layout.get_block_address(block_base, level);
            if (m_layout.block_has_buddy(block_address))
            {
                auto joint_block_address = layout_type::get_joint_block_address(block_address);
                m_state_tracker.toggle_joint_block_state(joint_block_address);
            }
            m_freelist_collection[level].push(block_base);
        }

        [[nodiscard]]
        constexpr
        std::byte *
        pop(level_type level)
        {
            if (m_freelist_collection[level].empty()) return nullptr;
            auto block_base = m_freelist_collection[level].pop();
            auto block_address = m_layout.get_block_address(block_base, level);
            if (m_layout.block_has_buddy(block_address))
            {
                auto joint_block_address = layout_type::get_joint_block_address(block_address);
                m_state_tracker.toggle_joint_block_state(joint_block_address);
            }
            return block_base;
        }

        // TODO: optimize
        [[nodiscard]]
        constexpr
        std::byte *
        merge_or_push(level_type level, std::byte * block_base)
        {
            auto block_address = m_layout.get_block_address(block_base, level);
            if (!m_layout.block_has_buddy(block_address))
            {
                m_freelist_collection[level].push(block_base);
                return nullptr;
            }
            else
            {
                auto joint_block_address = layout_type::get_joint_block_address(block_address);
                bool is_buddy_free = !m_state_tracker.toggle_joint_block_state(joint_block_address);
                if (is_buddy_free)
                {
                    auto buddy_address = layout_type::get_buddy_block_address(block_address);
                    std::byte * buddy_base = m_layout.get_block(buddy_address).base;
                    m_freelist_collection[level].remove(buddy_base);
                    return std::min(block_base, buddy_base); // return base of joint block
                }
                else
                {
                    m_freelist_collection[level].push(block_base);
                    return nullptr;
                }
            }
        }


        layout_type m_layout;
        state_memory_block_type m_state_memory; // used to store state (the bitmap)

        std::array<freelist_type, levels> m_freelist_collection; // track free blocks
        buddy_state_tracker m_state_tracker; // track buddy blocks
    };

    // TODO: storing the state memory block and also passing it to the bitmap is redundant.
    // both types will end up storing the same memory block.
    // furthermore, the state doesn't touch the state memory block, except through the bitmap.
    // perhaps a better approach would be to just forward the memory block to the bitmap.
    // we could use the same technique on the bitmap class (capture the block type, allow auto-free on destruction block types to work).

}