#pragma once

#include <chrono>


namespace dd99::memory::block_allocator::composite
{
    // NOTE: For timing and stats make sure to compose this way:
    //     Stats<Timing<Sub_Alloc_T>>
    // Otherwise you will time the allocation along with the stats computation
    // TODO: Not finished
    template <class Sub_Alloc_T>
    class Timing : public Sub_Alloc_T
    {
    public:
        Timing(Sub_Alloc_T &&sub_allocator)
            : Sub_Alloc_T(std::move(sub_allocator))
        { }

        memory::Block allocate(std::size_t requested_size)
        {
            const auto start = m_clock.now();
            auto r = Sub_Alloc_T::allocate(requested_size);
            const auto end = m_clock.now();

            const auto timing = end - start;

            m_last_allocation = start;
            m_last_allocation_duration = timing;

            update_last_operation(start, timing);

            return r;
        }

        void deallocate(const memory::Block &memory)
        {
            const auto start = m_clock.now();
            Sub_Alloc_T::deallocate(memory);
            const auto end = m_clock.now();

            const auto timing = end - start;

            m_last_deallocation_duration = timing;

            update_last_operation(start, timing);
        }

        void deallocate_all()
        {
            const auto start = m_clock.now();
            Sub_Alloc_T::deallocate_all();
            const auto end = m_clock.now();

            const auto timing = end - start;

            m_last_full_deallocation_duration = timing;
         
            update_last_operation(start, timing);
        }

        bool owns(void *memory) const { return Sub_Alloc_T::owns(memory); }
        bool owns(const memory::Block &memory) const { return Sub_Alloc_T::owns(memory); }

    private:
        std::chrono::steady_clock m_clock;

        std::chrono::steady_clock::time_point m_last_allocation;
        std::chrono::steady_clock::time_point m_last_deallocation;
        std::chrono::steady_clock::time_point m_last_full_deallocation;
        
        std::chrono::steady_clock::duration m_last_allocation_duration;
        std::chrono::steady_clock::duration m_last_deallocation_duration;
        std::chrono::steady_clock::duration m_last_full_deallocation_duration;
        
        // any operation
        std::chrono::steady_clock::time_point m_last_operation;
        std::chrono::steady_clock::duration m_last_operation_duration;
    
    private:
        void update_last_operation(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::duration duration)
        {
            m_last_operation = start;
            m_last_operation_duration = duration;
        }

        
    };
}
