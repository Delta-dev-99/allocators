#pragma once

#include <allocators/structures/blocks/memory_block.hpp>
#include <allocators/exception.hpp>


namespace dd99_allocators_namespace::block_allocator::composite
{
    // An allocator adaptor that throws when allocation fails
    template <class Sub_Alloc_T, bool Throwing_Deallocation = false>
    class Throwing
    {
    public:
        Throwing(Sub_Alloc_T sub_allocator)
            : m_sub_allocator{std::forward<Sub_Alloc_T>(sub_allocator)}
        { }

        Throwing(const Throwing&) = delete;
        Throwing(Throwing&&) = default;
        Throwing & operator=(const Throwing &) = delete;
        Throwing & operator=(Throwing &&) = delete;

    public:
        [[nodiscard]]
        block allocate(std::size_t requested_size, std::size_t requested_alignment = 1)
        {
            const auto r = m_sub_allocator.allocate(requested_size, requested_alignment);
            // TODO: Add exception description?
            if (!r) throw failed_allocation_exception{};
            return r;
        }

        void deallocate(const block &memory) 
        {
            if (!owns(memory))
            {
                if constexpr (Throwing_Deallocation)
                    throw memory_not_owned_exception{};
                else return;
            }
            return m_sub_allocator.deallocate(memory);
        }

        void deallocate_all() { m_sub_allocator.deallocate_all(); }

        bool owns(const std::byte * memory) const { return m_sub_allocator.owns(memory); }

        bool owns(const block &memory) const { return m_sub_allocator.owns(memory); }

    public:
        Sub_Alloc_T m_sub_allocator;
    };

}
