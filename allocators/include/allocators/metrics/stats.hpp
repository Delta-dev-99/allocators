#pragma once

#include <allocators/internal_structures/memory_block.hpp>

namespace dd99::memory::block_allocator::metrics
{
    template <class Sub_Alloc_T>
    class Stats : public Sub_Alloc_T
    {
    public:
        Stats(Sub_Alloc_T &&sub_allocator)
            : Sub_Alloc_T(std::move(sub_allocator))
        { }

    public:
        struct Stats_Data
        {
            enum Counted_Field
            {
                // The operations (total and failed)
                Allocation,
                Failed_Allocation,
                Deallocation,
                Deallocation_Miss, // when deallocating memory now owned by the allocator
                Full_Deallocation,
                // sizes
                Allocated_Size,
                Deallocated_Size,

                // the total number of counted fields
                Field_Count,
            };

            std::size_t total[Counted_Field::Field_Count] = {};
            double mean_allocation_request_size = 0;
        };

    public:
        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size)
        {
            const auto total_previous_requested_size = m_stats_data.mean_allocation_request_size * double(m_stats_data.total[Stats_Data::Allocation]);
            m_stats_data.mean_allocation_request_size = (total_previous_requested_size + double(requested_size)) / double(m_stats_data.total[Stats_Data::Allocation] + 1);

            ++m_stats_data.total[Stats_Data::Allocation];

            auto r = Sub_Alloc_T::allocate(requested_size);

            if (r)
                m_stats_data.total[Stats_Data::Allocated_Size] += r.size;
            else
                ++m_stats_data.total[Stats_Data::Failed_Allocation];

            return r;
        }

        void deallocate(const memory::Block &memory)
        {
            ++m_stats_data.total[Stats_Data::Deallocation];

            if (!Sub_Alloc_T::owns(memory))
            {
                ++m_stats_data.total[Stats_Data::Deallocation_Miss];
                return;
            }

            Sub_Alloc_T::deallocate(memory);

            m_stats_data.total[Stats_Data::Deallocated_Size] += memory.size;            
        }

        void deallocate_all()
        {
            const auto currently_allocated_size = m_stats_data.total[Stats_Data::Allocated_Size] - m_stats_data.total[Stats_Data::Deallocated_Size];

            Sub_Alloc_T::deallocate_all();

            m_stats_data.total[Stats_Data::Deallocated_Size] += currently_allocated_size;
            ++m_stats_data.total[Stats_Data::Full_Deallocation];
        }

        bool owns(std::byte *memory) const { return Sub_Alloc_T::owns(memory); }
        bool owns(const memory::Block &memory) const { return Sub_Alloc_T::owns(memory); }

    private:
        Stats_Data m_stats_data;

    public:
        const Stats_Data & get_stats() const
        { return m_stats_data; }

        void reset_stats()
        {
            m_stats_data = {};
        }

        // auto get_accumulated_deallocated_size() const
        // { return m_accumulated_allocated_size - m_currently_allocated_size; }

        // auto get_number_of_failed_allocations() const
        // { return m_number_of_allocations - m_number_of_successful_allocations; }

        // auto get_allocation_failure_percentage() const
        // { return static_cast<double>(get_number_of_failed_allocations()) / m_number_of_allocations; }
    };
}
