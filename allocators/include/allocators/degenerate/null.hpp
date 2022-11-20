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
        constexpr Null() noexcept {}

        constexpr
        Null(const memory::Block &) noexcept
        { }

    public:
        constexpr
        memory::Block
        allocate(std::size_t) const noexcept
        { return {}; }
        
        constexpr
        memory::Block
        allocate(std::size_t) noexcept
        { return {}; }


        constexpr
        void
        deallocate(const memory::Block &) const noexcept { }

        constexpr
        void
        deallocate(const memory::Block &) noexcept { }


        constexpr
        void
        deallocate_all() const noexcept { }

        constexpr
        void
        deallocate_all() noexcept { }


        constexpr
        bool
        owns(std::byte *) const noexcept
        { return false; }

        constexpr
        bool
        owns(const memory::Block &) const noexcept
        { return false; }
    };
}