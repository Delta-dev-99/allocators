#pragma once

#include <concepts>
#include <cstddef>



namespace dd99::memory::block_allocator::buddy_namespace
{

    // the buddy state implements the underlying data structure access and manipulation functions used by the buddy allocator. this separation allows for different strategies for managing the buddy system's internal state, such as using a bitmap and linked list, a fused structure for both, or other data structure architectures, without reimplementing the same core logic.
    template <class T>
    concept State_Concept = requires (T & t, std::size_t level, std::byte * block_address)
    {
        { t.push(level, block_address) }            -> std::same_as<void>; // called on allocation when a block is split. unconditionally adds the unused buddy half to the free list and updates state.
        { t.merge_or_push(level, block_address) }   -> std::same_as<std::byte *>; // called on deallocation. if the buddy is also free, take it and returning its address. if the buddy is not free, pushes the block, updates state, and returns nullptr.
        { t.pop(level) }                            -> std::same_as<std::byte *>; // called on allocation. take a block from this level. returns nullptr if level is empty.
        { t.has_free_blocks(level) }                -> std::convertible_to<bool>; // checks if there are any free blocks available at the specified level.
        { t.reset() }                               -> std::same_as<void>; // resets the data structures, which means that freelists are emptied and must be reconstructed.
    };

    // The algorithm for using the state interface on the buddy allocator is as follows:
    // 1. On allocation:
    //    a. The buddy allocator determines the appropriate level for the requested block size.
    //    b. It calls `state.pop(level)`.
    //    c. If the level is empty, it recurses up in levels trying to split a larger block. Else just return the block.
    //    d. The recursion uses `state.pop(level)` to find a block, until one is found or the levels are exhausted.
    //    e. When recursing back, call `state.push(level, block_address)` to push the unused block halves back to the freelist and update state.
    // 2. On deallocation:
    //   a. The buddy allocator determines the level of the block being deallocated.
    //   b. It calls `state.merge_or_push(level, block_address)`.
    //   c. If we get nullptr, then the buddy was not free, and the block was pushed to the free list. State is updated accordingly and the deallocation process is complete.
    //   d. If we get a block address, then the buddy was also free and we just got it (by address). State now doesn't have the buddy as free anymore. We then recurse up to the next level with the merged block.

    // The rationale for having both `push` and `merge_or_push` is that on allocation, we always know when we are splitting a block, so we can unconditionally push the buddy half to the free list (no buddy check). On deallocation, we want to try merging with the buddy if it's free (we need to check). This allows saving some operations on allocation.

}
