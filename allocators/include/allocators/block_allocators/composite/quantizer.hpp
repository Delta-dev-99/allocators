#pragma once

#include <allocators/structures/memory_block.hpp>
#include <cstddef>


namespace dd99::memory::block_allocator::composite
{
    template <std::size_t Step_Size, class Sub_Alloc_T>
    class Quantizer : public Sub_Alloc_T
    {
    public:
        Quantizer(Sub_Alloc_T &&sub_allocator)
            : Sub_Alloc_T(std::move(sub_allocator))
        { }

    public:
        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size)
        {
            const auto size_step_ceiling = (requested_size - 1) + Step_Size - ((requested_size - 1) % Step_Size);
            return Sub_Alloc_T::allocate(size_step_ceiling);
        }

        void deallocate(const memory::Block &memory) { return Sub_Alloc_T::deallocate(memory); }

        void deallocate_all() { Sub_Alloc_T::deallocate_all(); }

        bool owns(const std::byte * memory) const { return Sub_Alloc_T::owns(memory); }

        bool owns(const memory::Block &memory) const { return Sub_Alloc_T::owns(memory); }
    };
}
