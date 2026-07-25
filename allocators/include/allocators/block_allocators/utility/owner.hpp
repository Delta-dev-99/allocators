#pragma once

#include <allocators/structures/blocks/memory_block.hpp>


namespace dd99_allocators_namespace::block_allocator::composite
{
    // An allocator that owns the memory it controls.
    // Ussage: Wrap one of the basic allocators and an auto-aquired memory class.
    // NOTE: The memory block structure is copyed multiple times.
    // NOTE: This class can only be meaningfully used with the Heap_Block struct.
    // NOTE: This class also defeats the purpose of the library.
    // NOTE: The only reason this class is on the "composite" namespace is that it can be used for composition.
    template <class Aquire_Memory_T, class Sub_Allocator_T>
    class Owner
    {
    public:
        Owner()
            : m_memory()
            , m_sub_allocator(m_memory) // initializes the allocator by passing the acquired memory
        { }

        Owner(const Owner&) = delete;
        Owner(Owner&&) = default;
        Owner & operator=(const Owner &) = delete;
        Owner & operator=(Owner &&) = delete;

    public:
        [[nodiscard]]
        block allocate(std::size_t requested_size, std::size_t requested_alignment = 1)
        { return m_sub_allocator.allocate(requested_size, requested_alignment); }

        void deallocate(const block &memory)
        { m_sub_allocator.deallocate(memory); }

        void deallocate_all()
        { m_sub_allocator.deallocate_all(); }

        bool owns(const std::byte * memory) const
        { return m_sub_allocator.owns(memory); }

        bool owns(const block &memory) const
        { return m_sub_allocator.owns(memory); }

    public:
        Aquire_Memory_T m_memory;
        Sub_Allocator_T m_sub_allocator;
    };
}

