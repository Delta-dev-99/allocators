#pragma once

#include <exception>

namespace dd99::memory::block_allocator::composite
{
    // An allocator adaptor that throws when allocation fails
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
            // TODO: Add exception description?
            if (!r) throw std::bad_alloc{};
            return r;
        }

        void deallocate(const memory::Block &memory) 
        {
            if (!owns(memory)) return;
            return Sub_Alloc_T::deallocate(memory);
        }

        void deallocate_all() { Sub_Alloc_T::deallocate_all(); }

        bool owns(std::byte *memory) const { return Sub_Alloc_T::owns(memory); }

        bool owns(const memory::Block &memory) const { return Sub_Alloc_T::owns(memory); }
    };

}
