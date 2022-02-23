#pragma once

#include <exception>

namespace dd99::memory::block_allocator::composite
{
    template <class Sub_Alloc_T>
    class Throwing : public Sub_Alloc_T
    {
    public:
        Throwing(Sub_Alloc_T &&sub_allocator)
            : Sub_Alloc_T(std::move(sub_allocator))
        { }

    public:
        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size)
        {
            const auto r = Sub_Alloc_T::allocate(requested_size);
            if (!r) throw std::bad_alloc{};
            return r;
        }

        void deallocate(const memory::Block &memory) 
        {
            if (!owns(memory)) throw std::bad_alloc{};
            return Sub_Alloc_T::deallocate(memory);
        }

        void deallocate_all() { Sub_Alloc_T::deallocate_all(); }

        bool owns(void *memory) const { return Sub_Alloc_T::contains(memory); }

        bool owns(const memory::Block &memory) const { return Sub_Alloc_T::owns(memory); }
    };

}
