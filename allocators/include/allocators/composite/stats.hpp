#pragma once

#include <allocators/structures/memory_block.hpp>

namespace dd99::memory::block_allocator::composite
{
    template <class Sub_Alloc_T>
    class Stats : public Sub_Alloc_T
    {
    public:
        Stats(Sub_Alloc_T &&sub_allocator)
            : Sub_Alloc_T(std::move(sub_allocator))
        { }

        memory::Block allocate(std::size_t requested_size)
        {
            m_mean_allocation_size = (m_mean_allocation_size * m_number_of_allocations + requested_size) / (m_number_of_allocations + 1);
            ++m_number_of_allocations;

            auto r = Sub_Alloc_T::allocate(requested_size);

            if (r)
            {
                ++m_number_of_successful_allocations;
                m_accumulated_allocated_size += r.size;
                m_currently_allocated_size += r.size;
            }

            return r;
        }

        void deallocate(const memory::Block &memory)
        {
            ++m_number_of_deallocations;

            if (Sub_Alloc_T::owns(memory))
            {
                Sub_Alloc_T::deallocate(memory);

                m_currently_allocated_size -= memory.size;
                ++m_number_of_owned_memory_deallocations;
            }
        }

        void deallocate_all()
        {
            Sub_Alloc_T::deallocate_all();

            m_currently_allocated_size = 0;
            ++m_number_of_deallocations;
            ++m_number_of_owned_memory_deallocations;
        }

        bool owns(void *memory) const { return Sub_Alloc_T::owns(memory); }
        bool owns(const memory::Block &memory) const { return Sub_Alloc_T::owns(memory); }

    private:
        std::size_t m_number_of_allocations = 0;
        std::size_t m_number_of_successful_allocations = 0;
        std::size_t m_accumulated_allocated_size = 0;
        std::size_t m_number_of_deallocations = 0;
        std::size_t m_number_of_owned_memory_deallocations = 0; // number of deallocations of memory not owned by the allocator
        std::size_t m_currently_allocated_size = 0;
        double m_mean_allocation_size = 0;

    public:
        void reset_stats()
        {
            m_number_of_allocations = 0;
            m_number_of_successful_allocations = 0;
            m_accumulated_allocated_size = 0;
            m_number_of_deallocations = 0;
            m_number_of_owned_memory_deallocations = 0;
            m_currently_allocated_size = 0;
            m_mean_allocation_size = 0;
        }
        
        auto get_number_of_allocations() const { return m_number_of_allocations; }
        auto get_number_of_successful_allocations() const { return m_number_of_successful_allocations; }
        auto get_accumulated_allocated_size() const { return m_accumulated_allocated_size; }
        auto get_number_of_deallocations() const { return m_number_of_deallocations; }
        auto get_number_of_owned_memory_deallocations() const { return m_number_of_owned_memory_deallocations; }
        auto get_currently_allocated_size() const { return m_currently_allocated_size; }
        auto get_mean_allocation_size() const { return m_mean_allocation_size; }

        auto get_accumulated_deallocated_size() const
        { return m_accumulated_allocated_size - m_currently_allocated_size; }

        auto get_number_of_failed_allocations() const
        { return m_number_of_allocations - m_number_of_successful_allocations; }

        auto get_allocation_failure_percentage() const
        { return static_cast<double>(get_number_of_failed_allocations()) / m_number_of_allocations; }

        // does not actually report how many deallocations failed
        // but how many times attempted to deallocate memory not owned by the allocator
        auto get_number_of_miss_deallocations() const
        { return m_number_of_deallocations - m_number_of_owned_memory_deallocations; }
    };
}
