#pragma once

#include <allocators/basic/allocator.hpp>


namespace dd99::memory::block_allocator
{
    // Only handles contiguous memory
    class Stack : public Allocator
    {
    public:
        Stack(const Block &memory)
            : m_memory(memory)
            , m_current(reinterpret_cast<std::uintptr_t>(memory.base))
        { }
        
    public:
        Block allocate(std::size_t requested_size)
        {
            const auto used_size = reinterpret_cast<std::uintptr_t>(m_current) - reinterpret_cast<std::uintptr_t>(m_memory.base);
            const auto remaining_size = m_memory.size - used_size;
            if (remaining_size >= requested_size)
            {
                Block current{.base = reinterpret_cast<void *>(m_current), .size = requested_size};
                m_current += requested_size;
                return current;
            }

            return {};
        }

        // Can only free the last allocated block
        void deallocate(const Block &memory)
        {
            if (reinterpret_cast<std::uintptr_t>(memory.get_end()) == m_current)
            {
                m_current -= memory.size;
            }
        }

        void deallocate_all()
        {
            m_current = reinterpret_cast<std::uintptr_t>(m_memory.base);
        }

        bool owns(void *memory) const
        {
            return m_memory.contains(memory);
        }

        bool owns(const Block &memory) const
        {
            return m_memory.contains(memory);
        }

    private:
        Block m_memory;
        std::uintptr_t m_current;
    };
}
