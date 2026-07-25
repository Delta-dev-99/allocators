#pragma once

#include <allocators/structures/blocks/memory_block.hpp>



namespace dd99_allocators_namespace::block_allocator::utility
{
    // Wraps a reference to an allocator.
    // Requires an existing and persisting allocator instance.
    // This allows the use of the same allocator as an underlying
    // allocator of 2 or more composite allocators.
    // NOTE: see the `Ref` composite allocator.
    template <class Sub_Alloc_T>
    class XRef
    {
    public:
        XRef(Sub_Alloc_T &underlying_allocator)
            : m_alloc_ref(underlying_allocator)
        { }

        XRef(const XRef&) = default;
        XRef(XRef&&) = default;
        XRef & operator=(const XRef &) = delete;
        XRef & operator=(XRef &&) = delete;

    public:
        template <class Request>
        [[nodiscard]]
        block allocate(Request request)
        { return m_alloc_ref.allocate(request); }

        void deallocate(const block &memory)
        { m_alloc_ref.deallocate(memory); }

        void deallocate_all()
        { m_alloc_ref.deallocate_all(); }

        bool owns(const std::byte * memory) const
        { return m_alloc_ref.owns(memory); }

        bool owns(const block &memory) const
        { return m_alloc_ref.owns(memory); }

    public:
        Sub_Alloc_T & m_alloc_ref;
    };
}