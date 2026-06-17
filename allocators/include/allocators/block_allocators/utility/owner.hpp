#pragma once

#include <allocators/structures/blocks/memory_block.hpp>


namespace dd99::memory::block_allocator::composite
{
    // An allocator that owns the memory it controls.
    // Ussage: Wrap one of the basic allocators and an auto-aquired memory class.
    // NOTE: The memory block structure is copyed multiple times.
    // NOTE: This class can only be meaningfully used with the Heap_Block struct.
    // NOTE: This class also defeats the purpose of the library.
    // NOTE: The only reason this class is on the "composite" namespace is that it can be used for composition.
    template <class Aquire_Memory_T, class Sub_Allocator_T>
    class Owner : Aquire_Memory_T, public Sub_Allocator_T
    {
    public:
        Owner()
            : Aquire_Memory_T()
            , Sub_Allocator_T(*this) // initializes the allocator by passing this as a memory block
        { }

    public:
        [[nodiscard]]
        memory::block allocate(std::size_t requested_size, std::size_t requested_alignment = 1)
        { return Sub_Allocator_T::allocate(requested_size, requested_alignment); }

        void deallocate(const memory::block &memory)
        { Sub_Allocator_T::deallocate(memory); }

        void deallocate_all()
        { Sub_Allocator_T::deallocate_all(); }

        bool owns(const std::byte * memory) const
        { return Sub_Allocator_T::owns(memory); }

        bool owns(const memory::block &memory) const
        { return Sub_Allocator_T::owns(memory); }
    };
}

