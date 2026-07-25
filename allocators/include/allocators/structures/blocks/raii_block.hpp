#pragma once

#include <allocators/block_allocators/block_allocator.hpp>

namespace dd99_allocators_namespace
{

    // TODO: Find better name.
    // This is a block that holds a reference to an allocator.
    // It deallocates itself on destruction.
    // Same semantics as a unique_ptr.
    // Does not allow copy, only move.
    template <class Deallocator = void(*)(block)>
    struct raii_block : private block
    {
        // offer the same block API
        using block::base;
        using block::size;
        using block::contains;
        using block::get_base;
        using block::get_size;
        using block::get_end;
        using block::empty;
        using block::operator bool;

        ~raii_block()
        {
            if (!empty())
            {
                dealloc(get_block());
            }
        }

        raii_block() = delete;
        raii_block(block blk, Deallocator && deallocator) noexcept
            : block(blk)
            , dealloc(std::move(deallocator))
        { }

        raii_block(const raii_block &) = delete;
        raii_block(raii_block && other) noexcept
            : block(std::move(other))
            , dealloc(std::move(other.dealloc))
        {
            other.base = nullptr;
            other.size = 0;
        }

        raii_block & operator=(const raii_block &) = delete;
        raii_block & operator=(raii_block && other) noexcept
        {
            if (!empty()) dealloc(get_block());
            dealloc = std::move(other.dealloc);
            block::operator=(std::move(other));
            return *this;
        }

        [[nodiscard]]
        constexpr
        block
        get_block() const noexcept
        {
            return block{
                .base = get_base(),
                .size = get_size()
            };
        }

        constexpr
        block
        release() noexcept
        {
            auto blk = get_block();
            // base = nullptr;
            size = 0;
            // TODO: add something like `clear()` to the block interface
            return blk;
        }


        Deallocator dealloc;
    };



    template <Block_Allocator Allocator>
    constexpr auto
    allocate_raii_block(Allocator & allocator, std::size_t requested_size, std::size_t requested_alignment = 1) noexcept
    {
        return raii_block{allocator.allocate(requested_size, requested_alignment), [&](block blk){ allocator.deallocate(blk); }};
    }

}


// NOTE: currently this class inherits from block privately (different from before).
// this DOES NOT raise concerns about implicit decay, which would inadvertently cause the destructor to be called and the block to become invalid.
// perhaps we could check whether unique_ptr can be used where a pointer is expected.
// a better design may allow explicitly getting the block, implicitly working with the members, but not the implicit decay.

// NOTE: considering polymorphism on this class.
// the destructor is not virtual.
// the base class block doesn't have a virtual destructor.
// holding a `block *` to an instance of this class may be safe, but not deleting the pointer.
// destruction and assignment behavior is the only thing this class adds, but can't be used polymorphically.
// this means that there is no reason to allow polymorphism here.
