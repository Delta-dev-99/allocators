#pragma once

#include <allocators/basic/allocator.hpp>

namespace dd99::memory::block_allocator::composite
{
    template <std::size_t Threshold, class Allocator_LE, class Allocator_G>
    class Segregator_Allocator : public Allocator
    {
        Segregator_Allocator(Allocator_LE &&allocator_le, Allocator_G &&allocator_g)
            : m_alloc_le(std::move(allocator_le))
            , m_alloc_g(std::move(allocator_g))
        { }

    public:
        memory::Block allocate(std::size_t requested_size)
        {
            if (requested_size <= Threshold)
                return m_alloc_le::allocate(requested_size);
            else
                return m_alloc_g::allocate(requested_size);
        }

        void deallocate(const memory::Block &memory)
        {
            if (memory.size <= Threshold)
                m_alloc_le::deallocate(memory);
            else
                m_alloc_g::deallocate(memory);
        }

        void deallocate_all()
        {
            m_alloc_le::deallocate_all();
            m_alloc_g::deallocate_all();
        }

        bool owns(void *memory) const
        {
            // no size information
            return m_alloc_le::owns(memory) || m_alloc_g::owns(memory);
        }

        bool owns(const memory::Block &memory) const
        {
            return (memory.size <= Threshold) ? m_alloc_le::owns(memory) : m_alloc_g::owns(memory);
        }

    private:
        Allocator_LE m_alloc_le;
        Allocator_G m_alloc_g;
    };

}
