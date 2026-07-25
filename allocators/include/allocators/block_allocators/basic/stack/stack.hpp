#pragma once

#include <allocators/structures/blocks/block_concept.hpp>
#include <allocators/block_allocators/block_allocator.hpp>
#include <allocators/alignment.hpp>
#include <algorithm>
#include <bit>


namespace dd99_allocators_namespace::block_allocator
{
    // memory overhead on the controlled block: none
    template <std::size_t Natural_Alignment = 4, Movable_Block Block_Type = block>
    class Stack
    {
        // end of allocations is always aligned to this alignment.
        // this allows allocations without padding if `requested_alignment <= natural_alignment`
        // note: deallocation is unaware of padding, and only works if the block end matches the current pointer
        static constexpr auto natural_alignment = Natural_Alignment;

        static_assert(std::has_single_bit(natural_alignment),
            "alignment must be a power of 2");

    public:
        using block_type = Block_Type;

        struct mark_type
        {
            std::byte * m_current;
        };

    public:
        Stack(block_type memory)
            : m_memory(std::move(memory))
            , m_current(m_memory.get_base())
        { }

        Stack(const Stack &) = delete;
        Stack(Stack &&) = default;

        Stack & operator=(const Stack &) = delete;
        Stack & operator=(Stack &&) = default;
        
    public:
        [[nodiscard]]
        block allocate(std::size_t requested_size,
                       std::size_t requested_alignment = 1)
        {
            const auto aligned_current = align_up(m_current, requested_alignment); // add alignment padding
            const auto aligned_used_size = std::size_t(aligned_current - m_memory.get_base()); // used + alignment padding
            const auto available_aligned_size = m_memory.get_size() - aligned_used_size;
            if (available_aligned_size >= requested_size)
            {
                if constexpr (natural_alignment > 1)
                {
                    const auto aligned_end = align_up(aligned_current + requested_size, natural_alignment);
                    requested_size = std::min(available_aligned_size, static_cast<std::size_t>(aligned_end - aligned_current));
                }
                block allocated_block{.base = aligned_current, .size = requested_size};
                m_current = allocated_block.get_end();
                return allocated_block;
            }

            return {};
        }

        // Can only free the last allocated block
        void deallocate(const block &memory)
        {
            if (memory.base < m_memory.get_base()) return; // TODO: *** consider whether this check should be omitted. maybe this could be an assertion?
            if (memory.get_end() == m_current) // implies `owns()`
            {
                m_current -= memory.size;
            }
        }

        void deallocate_all()
        {
            m_current = m_memory.get_base();
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
        constexpr block align(std::size_t alignment)
        {
            auto old = m_current;
            m_current = align_up(m_current, alignment);
            return block{.base = old, .size = static_cast<std::size_t>(m_current - old)};
        }

    private:
        block_type m_memory;
        std::byte * m_current;
    };

    static_assert(Block_Allocator<Stack<>>, "This definition doesn't comply with the `Block_Allocator` concept");

}
