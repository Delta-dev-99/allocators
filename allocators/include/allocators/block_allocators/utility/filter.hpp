#pragma once

#include <allocators/block_allocators/block_allocator.hpp>

namespace dd99::memory::block_allocator::utility
{
    // An allocator that tests for a predicate on requests
    // Returns null block when predicate evaluates to false
    // NOTE: This is not an "Allocator" and cannot be used
    // as a polymorphic form of it or in compositing.
    // NOTE:
    //      *** memory size is still needed. It will be
    //      *** extracted via `get_size()` method from the request.
    //      *** Request type needs to provide `get_size()` member func
    template <class Request, class Predicate, class Sub_Allocator>
    class Filter
    {
    public:
        Filter(Sub_Allocator && sub_allocator, Predicate && predicate)
            : m_sub_allocator(std::move(sub_allocator))
            , m_predicate(std::move(predicate))
        { }

    public:
        [[nodiscard]]
        memory::block allocate(Request request)
        {
            if (m_predicate(request))
                return m_sub_allocator.allocate(request.get_size());
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
