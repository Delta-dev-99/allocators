#pragma once

#include <allocators/basic/allocator.hpp>
#include <allocators/structures/free_list.hpp>


namespace dd99::memory::block_allocator
{
    // Allocate fixed-size blocks
    // Uses a free list (forward list with nodes in unused blocks)
    // Requires Block_Size to be large enough to fit a list node in a block
    template <std::size_t Block_Size>
    class Pool : public Allocator
    {
    public:
        Pool(const memory::Block &memory)
            : m_memory(memory)
            , m_free_list(Block_Size)
        {
            build_free_list();
        }

    public:
        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size)
        {
            // larger allocations not supported
            if (requested_size > Block_Size)
                return {};
            
            return m_free_list.pop();
        }

        void deallocate(const memory::Block &memory)
        {
            if (m_memory.contains(memory))
                m_free_list.push(memory);
        }

        void deallocate_all()
        {
            m_free_list.clear();
            build_free_list();
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
        void build_free_list()
        {
            memory::Block current{.base = m_memory.base, .size = Block_Size};
            while(current.get_end() <= m_memory.get_end())
            {
                m_free_list.push(current);
                // advance
                current.base = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(current.base) + Block_Size);
            }
        }

    private:
        memory::Block m_memory;
        memory::structure::Freelist m_free_list;
    };

}
