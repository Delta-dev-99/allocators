#pragma once

#include <allocators/library_configuration/cpp_config.hpp>
#include <allocators/block_allocators/block_allocator.hpp>
#include <bit>

namespace dd99_allocators_namespace::block_allocator::composite
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
        Filter(Sub_Allocator sub_allocator, Predicate && predicate)
            : m_sub_allocator(std::forward<Sub_Allocator>(sub_allocator))
            , m_predicate(std::move(predicate))
        { }

        Filter(const Filter&) = delete;
        Filter(Filter&&) = default;
        Filter & operator=(const Filter &) = delete;
        Filter & operator=(Filter &&) = delete;

    public:
        [[nodiscard]]
        block allocate(std::size_t requested_size, std::size_t requested_alignment = 1)
        {
            DD99_ALLOCATORS_ASSERT_HARDENED("alignment must be power of 2", std::has_single_bit(requested_alignment));
            
            if (m_predicate(requested_size, requested_alignment))
                return m_sub_allocator.allocate(requested_size, requested_alignment);
            return {};
        }

        void deallocate(const block &memory)
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

        bool owns(const block &memory) const
        {
            return m_sub_allocator.owns(memory);
        }

    public:
        Sub_Allocator m_sub_allocator;

    private:
        Predicate m_predicate;
    };


}
