#pragma once

#include <allocators/block_allocators/block_allocator.hpp>



namespace dd99::memory::block_allocator::degenerate
{
    // TOOD: Find better name
    // An allocator that manages a single full block.
    // Allocates and deallocates all at once.
    // Has only 2 states: all allocated and all deallocated
    class Boolean
    {
    public:
        Boolean(const memory::block & memory)
            : m_memory(memory)
        { }

    public:
        [[nodiscard]]
        memory::block allocate(std::size_t /* requested_size */, std::size_t /* requested_alignment */ = 1)
        {
            if (m_allocated) return {};

            m_allocated = true;
            return m_memory;
        }

        void deallocate(const memory::block &memory)
        {
            if (owns(memory))
                m_allocated = false;
        }

        void deallocate_all()
        { m_allocated = false; }

        bool owns(const std::byte * memory) const
        { return m_memory.contains(memory); }

        bool owns(const memory::block& memory) const
        { return m_memory.contains(memory); }

    private:
        bool m_allocated = false;
        memory::block m_memory;
    };

    static_assert(Block_Allocator<Boolean>, "This definition doesn't comply with the `Block_Allocator` concept");

}
