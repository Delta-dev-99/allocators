#pragma once

#include <allocators/basic/allocator.hpp>



namespace dd99::memory::block_allocator
{
    // TODO: Find better name
    // A Constant allocator has a constant state.
    // Allocations always return the same memory.
    // Everything else is noop.
    class Constant final : public Allocator
    {
    public:
        constexpr
        Constant(const memory::Block & memory)
            : m_memory(memory)
        { }

    public:
        constexpr
        memory::Block
        allocate(std::size_t requested_size) const
        { return m_memory; }
        
        constexpr
        memory::Block
        allocate(std::size_t requested_size)
        { return m_memory; }


        constexpr
        void
        deallocate(const memory::Block &memory) const { }

        constexpr
        void
        deallocate(const memory::Block &memory) { }


        constexpr
        void
        deallocate_all() const { }

        constexpr
        void
        deallocate_all() { }


        constexpr
        bool
        owns(void *memory) const
        { return m_memory.contains(memory); }

        constexpr
        bool
        owns(const memory::Block& memory) const
        { return m_memory.contains(memory); }

    private:
        memory::Block m_memory;
    };
}