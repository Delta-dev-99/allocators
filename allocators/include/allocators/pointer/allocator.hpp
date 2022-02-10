#pragma once

#include <allocators/structures/memory_block.hpp>


namespace dd99::memory::pointer_allocator
{
    // Abstract base class
    // Base class for all allocators that return/take pointers
    struct Allocator
    {
        virtual ~Allocator() = default;

        Allocator() = default;
        Allocator(const Allocator &) = delete; // no copy
        Allocator(Allocator &&) = default; // move allowed

        [[nodiscard]]
        virtual void *allocate(std::size_t requested_size) = 0;
        
        virtual void deallocate(void *memory) = 0;
        virtual void deallocate_all() = 0;

        virtual bool owns(void *memory) const = 0;
        virtual bool owns(const Block &memory) const = 0;
    };

    
}
