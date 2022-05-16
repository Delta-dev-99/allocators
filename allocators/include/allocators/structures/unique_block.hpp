#pragma once

#include <allocators/structures/memory_block.hpp>

namespace dd99::memory
{

    // TODO: Find better name.
    // This is a block that holds a reference to an allocator.
    // It deallocates itself on destruction.
    // Same semantics as a unique_ptr.
    // Does not allow copy, only move.
    template <class Deallocator>
    struct Unique_Block : Block
    {
        Deallocator dealloc;

        ~Unique_Block()
        {
            dealloc(*this);
        }

        Unique_Block() = default;
        Unique_Block(Block && block, Deallocator && deallocator)
            : Block(std::move(block))
            , dealloc(std::move(deallocator))
        { }

        Unique_Block(const Unique_Block &) = delete;
        Unique_Block(Unique_Block && other)
            : Block(std::move(other))
            , dealloc(std::move(other.dealloc))
        {
            other.base = nullptr;
            other.size = 0;
        }

        Unique_Block & operator=(const Unique_Block &) = delete;
        Unique_Block & operator=(Unique_Block && other)
        {
            dealloc(*this);
            dealloc = std::move(other.dealloc);
            Block::operator=(std::move(other));
            return *this;
        }
        
    };

}
