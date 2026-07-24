#pragma once

#include <allocators/library_configuration/cpp_config.hpp>
#include <allocators/structures/blocks/memory_block.hpp>
#include <concepts>


namespace dd99::memory
{
    template <class Alloc>
    concept Block_Allocator = requires(Alloc alloc, std::size_t size, std::size_t alignment, const block & blk, const std::byte * b_ptr)
    {
        { alloc.allocate(size)              } -> std::same_as<block>;
        { alloc.allocate(size, alignment)   } -> std::same_as<block>;
        { alloc.deallocate(blk)             } -> std::same_as<void>;
        { alloc.deallocate_all()            } -> std::same_as<void>;
        { alloc.owns(blk)                   } -> std::same_as<bool>;
        { alloc.owns(b_ptr)                 } -> std::same_as<bool>;
    };
}

// namespace dd99::memory::block_allocator
// {
//     // abstract base class
//     // Base class for all allocators that return/take memory blocks
//     struct Allocator
//     {
//         virtual ~Allocator() = default;

//         Allocator() = default;
//         Allocator(const Allocator &) = delete; // no copy
//         Allocator(Allocator &&) = default; // move allowed

//         Allocator & operator=(const Allocator &) = delete;
//         Allocator & operator=(Allocator &&) = default;

//         [[nodiscard]]
//         virtual block allocate(std::size_t requested_size) = 0;
        
//         virtual void deallocate(const block &memory) = 0;
//         virtual void deallocate_all() = 0;

//         virtual bool owns(const std::byte * memory) const = 0;
//         virtual bool owns(const block &memory) const = 0;
        
//     };
// }
