#pragma once

#include <allocators/block_allocators/block_allocator.hpp>



namespace dd99::memory::block_allocator::degenerate
{
    // TODO: Find better name
    // A Null allocator ALWAYS fails.
    // It is perfect in the sense that it
    // always fails successfully.
    // NOTE: Is stateless
    class Null
    {
    public:
        constexpr Null() noexcept {}

        constexpr
        Null(const memory::block &) noexcept
        { }

    public:
        constexpr
        memory::block
        allocate(std::size_t, std::size_t = 1) const noexcept
        { return {}; }
        
        constexpr
        memory::block
        allocate(std::size_t) noexcept
        { return {}; }


        constexpr
        void
        deallocate(const memory::block &) const noexcept { }

        constexpr
        void
        deallocate(const memory::block &) noexcept { }


        constexpr
        void
        deallocate_all() const noexcept { }

        constexpr
        void
        deallocate_all() noexcept { }


        constexpr
        bool
        owns(const std::byte *) const noexcept
        { return false; }

        constexpr
        bool
        owns(const memory::block &) const noexcept
        { return false; }

    };

    static_assert(Block_Allocator<Null>, "This definition doesn't comply with the `Block_Allocator` concept");
    
}
