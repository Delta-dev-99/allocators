#pragma once

#include <allocators/structures/blocks/memory_block.hpp>
#include <cstddef>


namespace dd99_allocators_namespace::block_allocator::composite
{
    template <std::size_t Step_Size, class Sub_Alloc_T>
    class Quantizer
    {
    public:
        Quantizer(Sub_Alloc_T sub_allocator)
            : m_sub_allocator{std::forward<Sub_Alloc_T>(sub_allocator)}
        { }

        Quantizer(const Quantizer&) = delete;
        Quantizer(Quantizer&&) = default;
        Quantizer & operator=(const Quantizer &) = delete;
        Quantizer & operator=(Quantizer &&) = delete;

    public:
        [[nodiscard]]
        block allocate(std::size_t requested_size, std::size_t requested_alignment = 1)
        {
            const auto size_step_ceiling = (requested_size - 1) + Step_Size - ((requested_size - 1) % Step_Size);
            return m_sub_allocator.allocate(size_step_ceiling, requested_alignment);
        }

        void deallocate(const block &memory) { return m_sub_allocator.deallocate(memory); }

        void deallocate_all() { m_sub_allocator.deallocate_all(); }

        bool owns(const std::byte * memory) const { return m_sub_allocator.owns(memory); }

        bool owns(const block &memory) const { return m_sub_allocator.owns(memory); }

    public:
        Sub_Alloc_T m_sub_allocator;
    };
}
