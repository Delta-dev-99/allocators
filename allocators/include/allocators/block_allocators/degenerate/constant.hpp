#pragma once

#include <allocators/block_allocators/block_allocator.hpp>



namespace dd99_allocators_namespace::block_allocator::degenerate
{
    // TODO: Find better name
    // A Constant allocator has a constant state.
    // Allocations always return the same memory.
    // Everything else is noop.
    class Constant
    {
    public:
        constexpr
        Constant(const block & memory)
            : m_memory(memory)
        { }

        constexpr Constant(const Constant&) = delete;
        constexpr Constant(Constant&&) = default;
        constexpr Constant & operator=(const Constant &) = delete;
        constexpr Constant & operator=(Constant &&) = delete;

    public:
        constexpr
        block
        allocate(std::size_t /* requested_size */, std::size_t /* requested_alignment */ = 1) const
        { return m_memory; }
        
        constexpr
        block
        allocate(std::size_t /* requested_size */, std::size_t /* requested_alignment */ = 1)
        { return m_memory; }


        constexpr
        void
        deallocate(const block &/* memory */) const { }

        constexpr
        void
        deallocate(const block &/* memory */) { }


        constexpr
        void
        deallocate_all() const { }

        constexpr
        void
        deallocate_all() { }


        constexpr
        bool
        owns(const std::byte * memory) const
        { return m_memory.contains(memory); }

        constexpr
        bool
        owns(const block & memory) const
        { return m_memory.contains(memory); }

    private:
        block m_memory;
    };

    static_assert(Block_Allocator<Constant>, "This definition doesn't comply with the `Block_Allocator` concept");

}