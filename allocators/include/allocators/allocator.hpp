#pragma once

// NOTE: This is not the main inlcude header of the library.
// This header just defines a base class for all allocators to provide runtime polimorphysm
// via virtual functions.


#include <allocators/structures/memory_block.hpp>


namespace dd99::memory
{
    template <class T>
    concept Block_Allocator = requires(T t, std::size_t s, Block B, std::byte * b_ptr)
    {
        { t.allocate(s) } -> std::same_as<Block>;
        { t.deallocate(B) } -> std::same_as<void>;
        { t.deallocate_all() } -> std::same_as<void>;
        { t.owns(B) } -> std::same_as<bool>;
        { t.owns(b_ptr) } -> std::same_as<bool>;
    };
}

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

        Allocator & operator=(const Allocator &) = delete;
        Allocator & operator=(Allocator &&) = default;

        [[nodiscard]]
        virtual Block allocate(std::size_t requested_size) = 0;
        
        virtual void deallocate(const Block &memory) = 0;
        virtual void deallocate_all() = 0;

        virtual bool owns(std::byte *memory) const = 0;
        virtual bool owns(const Block &memory) const = 0;
        
    };
}
