#pragma once

#include <allocators/allocator.hpp>


namespace dd99::memory::block_allocator
{
    // memory overhead on the controlled block: none
    class Stack
    {
    public:
        Stack(const Block &memory)
            : m_memory(memory)
            , m_current(memory.base)
        { }

        Stack(const Stack &) = delete;
        Stack(Stack &&) = default;

        Stack & operator=(const Stack &) = delete;
        Stack & operator=(Stack &&) = default;
        
    public:
        [[nodiscard]]
        Block allocate(std::size_t requested_size)
        {
            const auto used_size = std::size_t(m_current - m_memory.base);
            const auto available_size = m_memory.size - used_size;
            if (available_size >= requested_size)
            {
                Block current{.base = m_current, .size = requested_size};
                m_current += requested_size;
                return current;
            }

            return {};
        }

        // Can only free the last allocated block
        void deallocate(const Block &memory)
        {
            if (memory.base < m_memory.base) return; // TODO: consider whether this check should be omitted
            if (memory.get_end() == m_current) // implies `owns()`
            {
                m_current -= memory.size;
            }
        }

        void deallocate_all()
        {
            m_current = m_memory.base;
        }

        bool owns(std::byte *memory) const
        {
            return m_memory.contains(memory);
        }

        bool owns(const Block &memory) const
        {
            return m_memory.contains(memory);
        }

    private:
        Block m_memory;
        std::byte * m_current;
    };

    static_assert(Block_Allocator<Stack>, "This definition doesn't comply with the `Block_Allocator` concept");

}
