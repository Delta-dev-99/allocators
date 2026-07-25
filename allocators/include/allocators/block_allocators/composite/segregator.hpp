#pragma once

#include <allocators/block_allocators/block_allocator.hpp>

namespace dd99_allocators_namespace::block_allocator::composite
{
    template <std::size_t Threshold, class Allocator_LE, class Allocator_G>
    class Segregator
    {
    public:
        Segregator(Allocator_LE allocator_le, Allocator_G allocator_g)
            : m_alloc_le(std::forward<Allocator_LE>(allocator_le))
            , m_alloc_g(std::forward<Allocator_G>(allocator_g))
        { }

        Segregator(const Segregator&) = delete;
        Segregator(Segregator&&) = default;
        Segregator & operator=(const Segregator &) = delete;
        Segregator & operator=(Segregator &&) = delete;

    public:
        [[nodiscard]]
        block allocate(std::size_t requested_size, std::size_t requested_alignment = 1)
        {
            if (requested_size <= Threshold)
                return m_alloc_le.allocate(requested_size, requested_alignment);
            else
                return m_alloc_g.allocate(requested_size, requested_alignment);
        }

        void deallocate(const block &memory)
        {
            if (memory.size <= Threshold)
                m_alloc_le.deallocate(memory);
            else
                m_alloc_g.deallocate(memory);
        }

        void deallocate_all()
        {
            m_alloc_le.deallocate_all();
            m_alloc_g.deallocate_all();
        }

        bool owns(const std::byte * memory) const
        {
            // no size information
            return m_alloc_le.owns(memory) || m_alloc_g.owns(memory);
        }

        bool owns(const block &memory) const
        {
            return (memory.size <= Threshold) ? m_alloc_le.owns(memory) : m_alloc_g.owns(memory);
        }

    public:
        Allocator_LE m_alloc_le;
        Allocator_G m_alloc_g;
    };

}
