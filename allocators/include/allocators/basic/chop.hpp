#pragma once

#include <allocators/basic/allocator.hpp>
#include <allocators/structures/free_list.hpp>


namespace dd99::memory::block_allocator
{
    // Allocates memory blocks by dividing a large enough free memory block (chopping the requested size).
    // Uses a free list (linked list with nodes in unused blocks)
    // Requires allocation sizes to be large enough to fit a list node (adjusted if not)
    // TODO: This is unfinished
    class Chop : public Allocator
    {
    public:
        Chop(const memory::Block& memory)
            : m_memory(memory)
        {
            m_free_list.push(m_memory);
        }

    public:
        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size)
        {
            return m_free_list.pop_chop(requested_size);
        }

        void deallocate(const memory::Block &memory)
        {
            if (m_memory.contains(memory))
                m_free_list.push(memory);
        }

        void deallocate_all()
        {
            m_free_list.clear();
            m_free_list.push(m_memory);
        }

        bool owns(void *memory) const
        {
            return m_memory.contains(memory);
        }

        bool owns(const memory::Block& memory) const
        {
            return m_memory.contains(memory);
        }

    private:
        memory::Block m_memory;
        memory::structure::Freelist_Sized m_free_list;
    };

}
