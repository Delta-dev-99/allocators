#pragma once

#include <allocators/basic/allocator.hpp>



namespace dd99::memory::block_allocator::degenerate
{
    // TODO: Find better name
    // A Failed allocator ALWAYS fails.
    // It is perfect in the sense that it
    // always fails successfully.
    class Failed final : public Allocator
    {
    public:
        constexpr
        Failed(const memory::Block &)
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
        owns(void *) const
        { return false; }

        constexpr
        bool
        owns(const memory::Block &) const
        { return false; }
    };
}