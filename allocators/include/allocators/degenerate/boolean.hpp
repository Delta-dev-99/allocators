#pragma once

#include <allocators/allocator.hpp>



namespace dd99::memory::block_allocator::degenerate
{
    // TOOD: Find better name
    // An allocator that manages a single full block.
    // Allocates and deallocates all at once.
    // Has only 2 states: all allocated and all deallocated
    class Boolean
    {
    public:
        Boolean(const memory::Block & memory)
            : m_memory(memory)
        { }

    public:
        [[nodiscard]]
        memory::Block allocate(std::size_t /* requested_size */)
        {
            if (m_allocated) return {};

            m_allocated = true;
            return m_memory;
        }

        void deallocate(const memory::Block &memory)
        {
            if (owns(memory))
                m_allocated = false;
        }

        void deallocate_all()
        { m_allocated = false; }

        bool owns(std::byte *memory) const
        { return m_memory.contains(memory); }

        bool owns(const memory::Block& memory) const
        { return m_memory.contains(memory); }

    private:
        bool m_allocated = false;
        memory::Block m_memory;
    };

    static_assert(Block_Allocator<Boolean>, "This definition doesn't comply with the `Block_Allocator` concept");

}
