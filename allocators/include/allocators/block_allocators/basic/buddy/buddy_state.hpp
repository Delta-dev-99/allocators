#pragma once

#include <allocators/block_allocators/basic/buddy/buddy_layout.hpp>
#include <concepts>
#include <cstddef>



namespace dd99::memory::block_allocator::buddy_namespace
{

    // the buddy state implements the underlying data structure access and manipulation functions used by the buddy allocator. this separation allows for different strategies for managing the buddy system's internal state, such as using a bitmap and linked list, a fused structure for both, or other data structure architectures, without reimplementing the same core logic.
    // TODO: add requirements on static constexpr values that must be present
    // TODO: add requirements on member types that must be present
    template <class T>
    concept State_Concept = requires { typename T::level_type; } // TODO: consider adding requirements on `level_type`, such as maybe being an unsigned integer type.
    && requires (T & t, T::level_type level, std::byte * block_base)
    {
        { t.push(level, block_base) }           -> std::same_as<void>; // called on allocation when a block is split. adds block to freelist and toggles state bit.
        { t.pop(level) }                        -> std::same_as<std::byte *>; // called on allocation. take block from freelist and toggle state bit. returns nullptr if level is empty.
        { t.merge_or_push(level, block_base) }  -> std::same_as<std::byte *>; // called on deallocation. toggle state bit. if the buddy is free, take it from freelist and return joint block address. if the buddy is not free, add block to freelist and returns nullptr.

        // { t.has_free_blocks(level) }            -> std::convertible_to<bool>; // checks if there are any free blocks available at the specified level. // not used by the allocator. checks are performed by calling `pop` directly.
        { t.reset() }                           -> std::same_as<void>; // resets the data structures.
    };


    // allocation pseudocode:
    // std::byte * allocate_impl(level)
    // {
    //     if (level > max_level) return nullptr; // condition for terminating recursion
    //     if (auto block_base = m_state.pop(level)) return block_base; // try direct allocation from level
    //     auto block_base = allocate_impl(level+1); // recursively get larger block
    //     if (!block_base) return nullptr; // allocation failed
    //     auto half_size = layout_type::get_block_size(level); // half the size of the larger block
    //     auto buddy_base = block_base + half_size; // calculate address of buddy block
    //     m_state.push(level, buddy_base); // push buddy back to freelist. this also updates state to reflect `block` as allocated, because `block` and `buddy` share the same state bit.
    //     return block_base;
    // }
    // 
    // block allocate(level) { return block{.base = allocate_impl(level), .size = layout_type::get_block_size(level)}; }

    // deallocation pseudocode:
    // void deallocate_impl(level, block_base)
    // {
    //     if (larger_block_base = m_state.merge_or_push(level, block_base))
    //         return deallocate_impl(level+1, larger_block_base);
    // }
    // 
    // void deallocate(block) { return deallocate_impl(layout_type::get_block_level(block.size), block.base); }

    // TODO: are we losing performance by separating the recursion into `deallocate_impl` instead of implementing it into the state?
    // TODO: would it be better to use an iterative approach instead of recursion? maybe something like:
    // ```cpp
    // while (block_base = state.merge_or_push(level, block_base)) {
    //     ++level;
    // }
    // ```
    // perhaps the compiler will optimize because the recursion is on the return statement.
    // such optimization prevents consuming stack as recursion deepens, achieving equivalence to iteration.

    // TODO: we could gain a tiny bit of performance if we avoid redundant conversions between addressing forms for blocks.
    // addressing forms are:
    // - {base, size} : level is calculated from size, index is calculated from base and level
    // - {base, level} : size is calculated from level, index is calculated from base and level
    // - {index, level} : size is calculated from level, base is calculated from level and index.
    // the first one {base, size} is the normal block representation in the whole library
    // the third one {level, index} is the internal representation of buddy block addresses
    // the second one {base, level} is an intermediate compromise used in the state interface.
    // there is also one extra representation {index, size} which could theoretically work, but isn't used anywhere.
    // *** currently our interface always uses the {base, level} representation even when it may not be optimal for all implementations.
    // maybe we could let the specific implementation determine the addressing scheme and conversions to be used?
    // we could add some member types to the interface. then we would either rely on implicit/explicit casts or add conversion functions to the interface.


}
