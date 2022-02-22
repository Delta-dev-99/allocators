#pragma once

// NOTE: This is not the main inlcude header of the library.
// You must include each header separately. (feature, not bug :P)
// This header just defines a base class for all allocator to provide runtime polimorphysm
// via virtual functions.


#include <allocators/structures/memory_block.hpp>


namespace dd99::memory::block_allocator
{
    // abstract base class
    // Base class for all allocators that return/take memory blocks
    struct Allocator
    {
        virtual ~Allocator() = default;

        Allocator() = default;
        Allocator(const Allocator &) = delete; // no copy
        Allocator(Allocator &&) = default; // move allowed

        [[nodiscard]]
        virtual Block allocate(std::size_t requested_size) = 0;
        
        virtual void deallocate(const Block &memory) = 0;
        virtual void deallocate_all() = 0;

        virtual bool owns(void *memory) const = 0;
        virtual bool owns(const Block &memory) const = 0;

        // Some types for metaprogramming follow
        enum Operation
        {
            Allocation,
            Deallocation,
            Full_Deallocation,
            Any,
            Operation_Count,
        };
    };
}
