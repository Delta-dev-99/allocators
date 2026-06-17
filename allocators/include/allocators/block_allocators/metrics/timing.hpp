#pragma once

#include <allocators/block_allocators/block_allocator.hpp>
#include <chrono>


namespace dd99::memory::block_allocator::metrics
{
    namespace detail
    {
        // NOTE: Timings do not subsume between them.
        // NOTE: Example: Timing for allocation does not include timing for failed allocations
        enum Timed_Operation
        {
            Successful_Allocation,
            Failed_Allocation,
            Successful_Deallocation,
            Deallocation_Miss,
            Full_Deallocation,
            Any,
            Operation_Count
        };

        template <bool Keep> struct Last_Time_Points { };
        template <> struct Last_Time_Points<true>
        {
            std::chrono::steady_clock::time_point last_time_point[Timed_Operation::Operation_Count]{};
        };

        template <bool Keep> struct Last_Durations { };
        template <> struct Last_Durations<true>
        {
            std::chrono::steady_clock::duration last_duration[Timed_Operation::Operation_Count]{};
        };

        template <bool Keep> struct Total_Durations { };
        template <> struct Total_Durations<true>
        {
            std::chrono::steady_clock::duration total_duration[Timed_Operation::Operation_Count]{};
        };

        template <bool Keep_Last_Time_Points, bool Keep_Last_Durations, bool Keep_Total_Durations>
        struct Timing_Data : Last_Time_Points<Keep_Last_Time_Points>, Last_Durations<Keep_Last_Durations>, Total_Durations<Keep_Total_Durations>
        { };
    }



    // NOTE: For timing and stats make sure to compose this way:
    //     Stats<Timing<Sub_Alloc_T>>
    // Otherwise you will time the allocation along with the stats computation
    template <class Sub_Alloc_T, bool Keep_Last_Time_Points = true, bool Keep_Last_Durations = true, bool Keep_Total_Durations = true>
    class Timing : public Sub_Alloc_T
    {
    public:
        using Timed_Operation = detail::Timed_Operation;

    public:
        Timing(Sub_Alloc_T &&sub_allocator)
            : Sub_Alloc_T(std::move(sub_allocator))
        { }

    public:
        using Timing_Data = detail::Timing_Data<Keep_Last_Time_Points, Keep_Last_Durations, Keep_Total_Durations>;

    public:
        [[nodiscard]]
        memory::block allocate(std::size_t requested_size, std::size_t requested_alignment = 1)
        {
            const auto start = std::chrono::steady_clock::now();
            auto r = Sub_Alloc_T::allocate(requested_size, requested_alignment);
            const auto end = std::chrono::steady_clock::now();

            const auto duration = end - start;
            
            if (r)
                update_operation_timings(Timed_Operation::Successful_Allocation, start, duration);
            else
                update_operation_timings(Timed_Operation::Failed_Allocation, start, duration);

            return r;
        }

        void deallocate(const memory::block &memory)
        {
            const auto start = std::chrono::steady_clock::now();
            Sub_Alloc_T::deallocate(memory);
            const auto end = std::chrono::steady_clock::now();

            const auto duration = end - start;
            
            if (owns(memory))
                update_operation_timings(Timed_Operation::Successful_Deallocation, start, duration);
            else
                update_operation_timings(Timed_Operation::Deallocation_Miss, start, duration);
        }

        void deallocate_all()
        {
            const auto start = std::chrono::steady_clock::now();
            Sub_Alloc_T::deallocate_all();
            const auto end = std::chrono::steady_clock::now();

            const auto duration = end - start;

            update_operation_timings(Timed_Operation::Full_Deallocation, start, duration);
        }

        bool owns(const std::byte * memory) const { return Sub_Alloc_T::owns(memory); }
        bool owns(const memory::block &memory) const { return Sub_Alloc_T::owns(memory); }

    private:
        Timing_Data m_timing_data;

    public:
        const Timing_Data & get_timing_data() const
        {
            return m_timing_data;
        }
        
        void reset_timings()
        {
            m_timing_data = {};
        }

    private:
        void update_operation_timings(Timed_Operation operation, std::chrono::steady_clock::time_point time_point, std::chrono::steady_clock::duration duration)
        {
            if constexpr (Keep_Last_Time_Points)
                m_timing_data.last_time_point[operation] = time_point;

            if constexpr (Keep_Last_Durations)
                m_timing_data.last_duration[operation] = duration;

            if constexpr (Keep_Total_Durations)
                m_timing_data.total_duration[operation] += duration;

            if (operation != Timed_Operation::Any)
                update_operation_timings(Timed_Operation::Any, time_point, duration);
        }
        
    };
}
