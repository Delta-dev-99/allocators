#pragma once

#include <allocators/block_allocators/block_allocator.hpp>
#include <allocators/alignment.hpp>


namespace dd99::memory::block_allocator
{
    // memory overhead on the controlled block: none
    class Stack
    {
    public:
        struct mark_type
        {
            std::byte * m_current;
        };

    public:
        Stack(const block &memory)
            : m_memory(memory)
            , m_current(memory.base)
        { }

        Stack(const Stack &) = delete;
        Stack(Stack &&) = default;

        Stack & operator=(const Stack &) = delete;
        Stack & operator=(Stack &&) = default;
        
    public:
        [[nodiscard]]
        block allocate(std::size_t requested_size,
                    //    std::size_t requested_alignment = alignof(std::max_align_t))
                       std::size_t requested_alignment = 1)
        {
            const auto aligned_current = align_up(m_current, requested_alignment); // add alignment padding
            const auto used_size = std::size_t(aligned_current - m_memory.base); // used + alignment padding
            const auto available_size = m_memory.size - used_size;
            if (available_size >= requested_size)
            {
                block current{.base = m_current, .size = requested_size};
                m_current += requested_size;
                return current;
            }

            return {};
        }

        // Can only free the last allocated block
        void deallocate(const block &memory)
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

        bool owns(const std::byte * memory) const
        {
            return m_memory.contains(memory);
        }

        bool owns(const block & memory) const
        {
            return m_memory.contains(memory);
        }

    public: // stack-specific API
        [[nodiscard]] constexpr mark_type mark() noexcept { return mark_type{m_current}; }
        constexpr void reset(mark_type mark) noexcept { m_current = mark.m_current; }

        // consumes the stack until the base is aligned. Returns the consumed block (which may be empty).
        constexpr memory::block align(std::size_t alignment)
        {
            auto old = m_current;
            m_current = align_up(m_current, alignment);
            return memory::block{.base = old, .size = static_cast<std::size_t>(m_current - old)};
        }

    private:
        block m_memory;
        std::byte * m_current;
    };

    static_assert(Block_Allocator<Stack>, "This definition doesn't comply with the `Block_Allocator` concept");

}
