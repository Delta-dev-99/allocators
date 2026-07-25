#pragma once

#include <allocators/library_configuration/cpp_config.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_layout.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_state.hpp>
#include <allocators/structures/linked_list.hpp>
#include <allocators/structures/bitmap.hpp>
#include <array>



namespace dd99_allocators_namespace::block_allocator::buddy_namespace
{
    // fw-decl
    template <Layout_Concept Layout,
              class State_Memory_Block_Type,
              class Bitmap_Element_Type = std::byte>
    struct buddy_intrusive_state;


    template <Layout_Concept Layout, class Bitmap_Element_Type = std::byte>
    struct buddy_intrusive_state_traits
    {
        using layout_type = Layout;
        using bitmap_element_type = Bitmap_Element_Type;
        using bitmap_type = dd99_allocators_namespace::structure::Bitmap<bitmap_element_type>;

        static constexpr
        std::size_t
        get_state_size(const layout_type & layout)
        {
            const auto state_bits = layout.get_total_joint_block_count();
            const auto bitmap_size = bitmap_type::calculate_block_count(state_bits);
            return bitmap_size * bitmap_type::block_size;
        }

        static constexpr
        std::size_t
        get_state_alignment()
        {
            return bitmap_type::block_alignment;
        }

        template <class State_Memory_Block_Type>
        [[nodiscard]]
        static constexpr
        auto
        make_state(layout_type && layout, State_Memory_Block_Type && state_block)
            -> buddy_intrusive_state<layout_type, std::decay_t<State_Memory_Block_Type>, bitmap_element_type>
        {
            return {std::move(layout), std::forward<State_Memory_Block_Type>(state_block)};
        }

        struct split_block
        {
            block managed, state;
        };

        // TODO:
        static constexpr
        split_block
        calculate_block_split(block blk)
        {
            // // check if memory is not enough for 1 block + minimal bitmap
            // if (memory_size < bitmap_type::block_size + block_size)
            //     return 0;

            // // Analytical initial guess from continuous relaxation
            // constexpr auto pow2_Lm1 = 1ULL << (Levels - 1);
            // constexpr auto a_num = pow2_Lm1 - 1;          // 2^{L-1} - 1
            // constexpr auto denom = 8ULL * block_size * pow2_Lm1 + a_num;
            // // Numerator: 8 * memory_size * pow2_Lm1
            // // We assume memory_size * 8 * pow2_Lm1 < 2^64 (holds for realistic RAM).

            // // Ensure memory_size doesn't cause overflow: memory_size * 8 * pow2_Lm1 < 2^64
            // constexpr auto max_supported_memory = std::numeric_limits<std::size_t>::max() / (8ULL * pow2_Lm1);
            // assert(memory_size <= max_supported_memory && "memory_size would cause overflow in calculation");
            
            // // Calculate initial block count guess
            // std::size_t block_count = (8ULL * memory_size * pow2_Lm1) / denom;

            // // helper predicate
            // auto fits = [&](std::size_t block_count) -> bool {
            //     std::size_t bits = Buddy_Base::calculate_buddy_bit_count(block_count);
            //     std::size_t bitmap = BMP::calculate_block_count(bits) * BMP::block_size;
            //     return block_count * block_size + bitmap <= memory_size;
            // };

            // // adjust - at most ceil(BMP::block_size / block_size) steps
            // if (fits(block_count))
            // {
            //     while(fits(block_count + 1)) ++block_count;
            // }
            // else
            // {
            //     while(!fits(--block_count)); // decrement until fits
            // }

            // return block_count;
        }
    };

    // The intrusive state implementation stores linked list nodes directly on the managed free blocks, which helps reduce the memory footprint.
    // An external XOR bitmap is used to store buddy state. The user must provide the memory block for this bitmap.
    // 
    // The `LEVELS` template parameter:
    // This parameter determines the number of linked lists to create and the number of buddy states to manage for a given number of lowest-level blocks.
    // 
    // The `Block_Address_Type` template parameter:
    // The type used to represent the address of a block in the buddy system. Can be customized to balance required bookkeeping memory vs representable memory range for a given block_size used in the buddy allocator. This type is expected to encode both the level and index of the block within the buddy system's hierarchy. The default type is `dd99_allocators_namespace::block_allocator::buddy_policy::block_address<>`, which uses unsigned integers for both level and index. This type is used by the policy to identify blocks when performing operations such as pushing, popping, and merging blocks in the buddy system. The user can provide a custom type that satisfies the expected interface if they want to use a different encoding for block addresses.
    // 
    // The `State_Block_Type` template parameter:
    // The type of the memory block provided by the user to store the buddy state and free list information. Intended to be deduced via factory function. This block is expected to be large enough to hold the necessary data structures for managing the buddy system's state across all levels. The policy will use this block to maintain the state of which blocks are free and which are allocated, as well as to manage the linked lists of free blocks at each level. The user is responsible for providing a suitable memory block for this purpose, and the policy will handle the organization and management of this block internally.
    // This is part of the lifetime management customization mechanism used in this library. Allows either using a plain memory block, or a RAII auto-freeing memory block type.
    template <Layout_Concept Layout,
              class State_Memory_Block_Type,
              class Bitmap_Element_Type>
    struct buddy_intrusive_state
    {
        // Check whether this policy type actually satisfies the requirements for buddy policy types.
        // This is just a cost-free sanity check which should always pass.
        // Failure here likely means that either a template parameter was terribly wrong, or that there's an error in this implementation.
        // static_assert(State_Concept<buddy_intrusive_state>,
        //     "buddy_intrusive_state does not satisfy State_Concept requirements.");

        using layout_type = Layout;
        using state_memory_block_type = State_Memory_Block_Type;

        using block_address_type = layout_type::block_address_type;
        using level_type = block_address_type::level_type;
        using index_type = block_address_type::index_type;

        static constexpr auto levels = layout_type::levels;
        static constexpr auto block_size = layout_type::block_size;

        static constexpr auto last_level = levels - 1;

        using freelist_type = dd99_allocators_namespace::structure::basic_linked_list;
        using bitmap_type = dd99_allocators_namespace::structure::Bitmap<Bitmap_Element_Type>;

        static_assert(sizeof(freelist_type::node) <= block_size,
            "block_size is too small to hold a freelist node.");
        
        static_assert(block_size % alignof(freelist_type::node) == 0,
            "block_size must be a multiple of the freelist node alignment, "
            "so that every block meets alignment requirements for nodes");

        // a type for tracking the state of buddy blocks
        // wraps a bitmap and adds a convenient interface for the buddy allocator
        struct buddy_state_tracker
        {
            [[nodiscard]]
            constexpr
            std::size_t
            get_bitmap_index_from_joint(block_address_type joint_blk_address, const layout_type & layout) const
            {
                DD99_ALLOCATORS_ASSERT_DEBUG("blocks at level 0 aren't joint blocks", joint_blk_address.level > 0);

                return joint_blk_address.index + layout.get_cumulative_joint_block_count(joint_blk_address.level - 1);
            }

            // returns new state
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
        buddy_intrusive_state(layout_type && layout, state_memory_block_type state_memory)
            : m_layout{std::forward<layout_type>(layout)}
            , m_state_memory{std::move(state_memory)}
            , m_state_tracker{bitmap_type{m_layout.get_total_joint_block_count(), m_state_memory.get_base()}}
        {
            DD99_ALLOCATORS_ASSERT_HARDENED("managed memory base must be appropriately aligned to hold freelist nodes", is_aligned(m_layout.m_memory.get_base(), alignof(freelist_type::node)));
            DD99_ALLOCATORS_ASSERT_HARDENED("state memory too small", m_state_memory.get_size() >= bitmap_type::calculate_block_count(m_layout.get_total_joint_block_count()) * bitmap_type::block_size);
            DD99_ALLOCATORS_ASSERT_HARDENED("state memory must be appropriately aligned", is_aligned(m_state_memory.get_base(), bitmap_type::block_alignment));

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
                m_state_tracker.toggle_joint_block_state(joint_block_address, m_layout);
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
                m_state_tracker.toggle_joint_block_state(joint_block_address, m_layout);
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
                bool is_buddy_free = !m_state_tracker.toggle_joint_block_state(joint_block_address, m_layout);
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



// example. creation of state:
// 
// auto layout = make_buddy_layout<64, 6>(main_memory);
// 
// using state_traits_type = buddy_intrusive_state_traits<decltype(layout)>;
// auto state_size = state_traits_type::get_state_size(layout);
// auto state_alignment = state_traits_type::get_state_alignment();
//
// auto state_memory_block = allocate_state_memory_block_somehow(state_size, state_alignment);
// 
// auto state = state_traits_type::make_state(std::move(layout), std::move(state_memory_block));
// 
