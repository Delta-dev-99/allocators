#pragma once

#include <allocators/block_allocators/block_allocator.hpp>

namespace dd99::memory::block_allocator::composite
{
    // An allocator that tests for a predicate on requests
    // Returns null block when predicate evaluates to false
    // 
    // NOTE: There is another Filter that allows for
    // extended requests types, on the utility namespace.
    // It is not an "Allocator" though, as it will not take
    // just a size for allocation requests.
    template <class Predicate, class Sub_Allocator>
    class Filter
    {
    public:
        Filter(Sub_Allocator && sub_allocator, Predicate && predicate)
            : m_sub_allocator(std::move(sub_allocator))
            , m_predicate(std::move(predicate))
        { }

    public:
        [[nodiscard]]
        memory::block allocate(std::size_t requested_size)
        {
            if (m_predicate(requested_size))
                return m_sub_allocator.allocate(requested_size);
            return {};
        }

        void deallocate(const memory::block &memory)
        {
            m_sub_allocator.deallocate(memory);
        }

        void deallocate_all()
        {
            m_sub_allocator.deallocate_all();
        }

        bool owns(const std::byte * memory) const
        {
            return m_sub_allocator.owns(memory);
        }

        bool owns(const memory::block &memory) const
        {
            return m_sub_allocator.owns(memory);
        }

    private:
        Sub_Allocator m_sub_allocator;
        Predicate m_predicate;
    };


}
