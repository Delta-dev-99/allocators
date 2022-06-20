#pragma once

#include <allocators/allocator.hpp>



namespace dd99::memory::block_allocator::degenerate
{
    // TODO: Find better name
    // A Null allocator ALWAYS fails.
    // It is perfect in the sense that it
    // always fails successfully.
    // NOTE: Is stateless
    class Null : public Allocator
    {
    public:
        constexpr
        Null(const memory::Block &)
        { }

    public:
        constexpr
        memory::Block
        allocate(std::size_t) const
        { return {}; }
        
        constexpr
        memory::Block
        allocate(std::size_t)
        { return {}; }


        constexpr
        void
        deallocate(const memory::Block &) const { }

        constexpr
        void
        deallocate(const memory::Block &) { }


        constexpr
        void
        deallocate_all() const { }

        constexpr
        void
        deallocate_all() { }


        constexpr
        bool
        owns(std::byte *) const
        { return false; }

        constexpr
        bool
        owns(const memory::Block &) const
        { return false; }
    };
}