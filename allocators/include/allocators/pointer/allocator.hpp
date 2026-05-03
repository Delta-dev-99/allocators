#pragma once

#include <allocators/structures/memory_block.hpp>

namespace dd99::memory
{
    template <class T>
    concept Pointer_Allocator = requires(T t, std::size_t s, Block B, const std::byte * b_ptr)
    {
        { t.allocate(s) } -> std::same_as<std::byte *>;
        { t.deallocate(b_ptr) } -> std::same_as<void>;
        { t.deallocate_all() } -> std::same_as<void>;
        { t.owns(B) } -> std::same_as<bool>;
        { t.owns(b_ptr) } -> std::same_as<bool>;
    };
}

namespace dd99::memory::pointer_allocator
{
    // Abstract base class
    // Base class for all allocators that return/take pointers
    // struct Allocator
    // {
    //     virtual ~Allocator() = default;

    //     Allocator() = default;
    //     Allocator(const Allocator &) = delete; // no copy
    //     Allocator(Allocator &&) = default; // move allowed

    //     [[nodiscard]]
    //     virtual std::byte * allocate(std::size_t requested_size) = 0;
        
    //     virtual void deallocate(std::byte * memory) = 0;
    //     virtual void deallocate_all() = 0;

    //     virtual bool owns(const std::byte * memory) const = 0;
    //     virtual bool owns(const Block & memory) const = 0;
    // };

    
}
