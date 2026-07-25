#pragma once

#include <allocators/structures/blocks/memory_block.hpp>

namespace dd99_allocators_namespace
{
    template <class T>
    concept Pointer_Allocator = requires(T t, std::size_t s, block B, std::byte * b_ptr)
    {
        { t.allocate(s) } -> std::same_as<std::byte *>;
        { t.allocate(s, s) } -> std::same_as<std::byte *>; // aligned allocation
        { t.deallocate(b_ptr) } -> std::same_as<void>;
        { t.deallocate_all() } -> std::same_as<void>;
        { t.owns(B) } -> std::same_as<bool>;
        { t.owns(b_ptr) } -> std::same_as<bool>;
    };
}

namespace dd99_allocators_namespace::pointer_allocator
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
    //     virtual bool owns(const block & memory) const = 0;
    // };

    
}
