#pragma once

// #include <allocators/block_allocators/basic/buddy/buddy_layout.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_state.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_block_address.hpp>
#include <allocators/structures/blocks/memory_block.hpp>



namespace dd99::memory::block_allocator::buddy_namespace
{

    // The fused external buddy policy is a policy for the buddy allocator that uses a single, fused data structure to manage both the free list and the buddy state.
    // This policy is designed to be more performant by taking advantage of the fact that most buddy operations need to access both the free list and the buddy state for the same block at the same time.
    // By using a single data structure, we can reduce the number of memory accesses and improve cache locality, which can lead to faster allocation and deallocation times.
    // The fused external buddy policy is particularly effective in scenarios where the buddy allocator is used in performance-critical applications, such as real-time systems or high-performance computing environments.
    // This policy aims to provide a Linux-style buddy allocator.
    // 
    // The `LEVELS` template parameter:
    // This parameter determines the number of linked lists to create and the number of buddy states to manage for a given number of lowest-level blocks.
    // 
    // The `Block_Address_Type` template parameter:
    // The type used to represent the address of a block in the buddy system. Can be customized to balance required bookkeeping memory vs representable memory range for a given Block_Size used in the buddy allocator. This type is expected to encode both the level and index of the block within the buddy system's hierarchy. The default type is `dd99::memory::block_allocator::buddy_policy::block_address<>`, which uses unsigned integers for both level and index. This type is used by the policy to identify blocks when performing operations such as pushing, popping, and merging blocks in the buddy system. The user can provide a custom type that satisfies the expected interface if they want to use a different encoding for block addresses.
    // 
    // The `State_Memory_Block_Type` template parameter:
    // The type of the memory block provided by the user to store the buddy state and free list information. Intended to be deduced via factory function. This block is expected to be large enough to hold the necessary data structures for managing the buddy system's state across all levels. The policy will use this block to maintain the state of which blocks are free and which are allocated, as well as to manage the linked lists of free blocks at each level. The user is responsible for providing a suitable memory block for this purpose, and the policy will handle the organization and management of this block internally.
    // This is part of the lifetime management customization mechanism used in this library. Allows either using a plain memory block, or a RAII auto-freeing memory block type.
    template <Layout_Concept Layout,
              class State_Memory_Block_Type>
    struct buddy_fused_external_state
    {
        // Check whether this policy type actually satisfies the requirements for buddy policy types.
        // This is just a cost-free sanity check which should always pass.
        // Failure here likely means that either a template parameter was terribly wrong, or that there's an error in this implementation.
        static_assert(State_Concept<buddy_fused_external_state>,
            "buddy_fused_external_state does not satisfy State_Concept requirements.");

        using layout_type = Layout;
        using block_type = State_Memory_Block_Type;
        using block_address_type = layout_type::block_address_type;
        using level_type = block_address_type::level_type;
        using index_type = block_address_type::index_type;



        block_type m_memory; // used to store state (buddy state and linked list of free blocks per level)



        void push(level_type level, std::byte * block_address)
        {
            // TODO: implementation of push for fused external policy
        }
    };

}