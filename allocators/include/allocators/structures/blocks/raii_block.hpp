#pragma once

#include <allocators/structures/blocks/memory_block.hpp>

namespace dd99::memory
{

    // TODO: Find better name.
    // This is a block that holds a reference to an allocator.
    // It deallocates itself on destruction.
    // Same semantics as a unique_ptr.
    // Does not allow copy, only move.
    template <class Deallocator>
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
            // NOTE: allocator checks block before deallocating
            dealloc(*this);
        }

        raii_block() = default;
        raii_block(block && blk, Deallocator && deallocator)
            : block(std::move(blk))
            , dealloc(std::move(deallocator))
        { }

        raii_block(const raii_block &) = delete;
        raii_block(raii_block && other)
            : block(std::move(other))
            , dealloc(std::move(other.dealloc))
        {
            other.base = nullptr;
            other.size = 0;
        }

        raii_block & operator=(const raii_block &) = delete;
        raii_block & operator=(raii_block && other)
        {
            dealloc(*this);
            dealloc = std::move(other.dealloc);
            block::operator=(std::move(other));
            return *this;
        }

        constexpr
        block
        get_block() const noexcept
        {
            return block{
                .base = get_base(),
                .size = get_size()
            };
        }


        Deallocator dealloc;
    };

}


// TODO: currently this class inherits from block and can be used interchangeably.
// this raises concerns about implicit decay, which would inadvertently cause the destructor to be called and the block to become invalid.
// perhaps we should check whether unique_ptr can be used where a pointer is expected.
// a better design may allow explicitly getting the block, implicitly working with the members, but not the implicit decay.

// TODO: consider polymorphism on this class.
// the destructor is not virtual.
// the base class block doesn't have a virtual destructor.
// holding a `block *` to an instance of this class may be safe, but not deleting the pointer.
// destruction and assignment behavior is the only thing this class adds, but can't be used polymorphically.
// this means that there is no reason to allow polymorphism here.
