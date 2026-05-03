#pragma once

#include <allocators/structures/memory_block.hpp>
#include <allocators/exception.hpp>


namespace dd99::memory::block_allocator::composite
{
    // An allocator adaptor that throws when allocation fails
    template <class Sub_Alloc_T, bool Throwing_Deallocation = false>
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
            if (!r) throw dd99::memory::failed_allocation_exception{};
            return r;
        }

        void deallocate(const memory::Block &memory) 
        {
            if (!owns(memory))
            {
                if constexpr (Throwing_Deallocation)
                    throw dd99::memory::memory_not_owned_exception{};
                else return;
            }
            return Sub_Alloc_T::deallocate(memory);
        }

        void deallocate_all() { Sub_Alloc_T::deallocate_all(); }

        bool owns(const std::byte * memory) const { return Sub_Alloc_T::owns(memory); }

        bool owns(const memory::Block &memory) const { return Sub_Alloc_T::owns(memory); }
    };

}
