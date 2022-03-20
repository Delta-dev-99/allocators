#pragma once

#include <allocators/internal/bases/allocator.hpp>

namespace dd99::memory::block_allocator::composite
{
    // An allocator that wraps a reference to another allocator.
    // Requires an existing and persisting allocator instance.
    // This allows the use of the same allocator as an underlying
    // allocator of 2 or more composite allocators.
    template <class Sub_Alloc_T>
    class Ref : public Allocator
    {
    public:
        Ref(Sub_Alloc_T &underlying_allocator)
            : m_alloc_ref(underlying_allocator)
        { }

    public:
        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size)
        { return m_alloc_ref.allocate(requested_size); }

        void deallocate(const memory::Block &memory)
        { m_alloc_ref.deallocate(memory); }

        void deallocate_all()
        { m_alloc_ref.deallocate_all(); }

        bool owns(std::byte *memory) const
        { return m_alloc_ref.owns(memory); }

        bool owns(const memory::Block &memory) const
        { return m_alloc_ref.owns(memory); }

    protected:
        Sub_Alloc_T &m_alloc_ref;
    };
}