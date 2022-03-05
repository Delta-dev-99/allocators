#pragma once


namespace dd99::memory::block_allocator::composite
{
    // An allocator that owns the memory it controls.
    // Ussage: Wrap one of the basic allocators and an auto-aquired memory class.
    // NOTE: The memory Block structure is copyed multiple times.
    // NOTE: This class can only be meaningfully used with the Heap_Block struct.
    // NOTE: This class also defeats the purpose of the library.
    template <class Aquire_Memory_T, class Sub_Allocator_T>
    class Owner : Aquire_Memory_T, public Sub_Allocator_T
    {
    public:
        Owner()
            : Aquire_Memory_T()
            , Sub_Allocator_T(*this)
        { }

    public:
        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size)
        { return Sub_Allocator_T::allocate(requested_size); }

        void deallocate(const memory::Block &memory)
        { Sub_Allocator_T::deallocate(memory); }

        void deallocate_all()
        { Sub_Allocator_T::deallocate_all(); }

        bool owns(std::byte *memory) const
        { return Sub_Allocator_T::owns(memory); }

        bool owns(const memory::Block &memory) const
        { return Sub_Allocator_T::owns(memory); }
    };
}

