#pragma once

#include <allocators/pointer_allocators/allocator.hpp>
#include <new>

namespace dd99::memory::pointer_allocator
{
    // Allows the allocation and deallocation of memory using pointers
    // This allocator is the bridge between this library and standard c++ allocation
    // NOTE that this is not a composable allocator,
    // nor compatible with the rest of allocators on this library.
    template <class Sub_Alloc_T>
    class Basic
    {
    public:
        using sub_allocator_type = Sub_Alloc_T;

    public:
        Basic(sub_allocator_type sub_allocator)
            : m_sub_alloc(std::move(sub_allocator))
        { }

    private:
        sub_allocator_type m_sub_alloc;
    
    public:
        [[nodiscard]]
        std::byte * allocate(std::size_t requested_size, std::size_t requested_alignment = 1)
        {
            // TODO: *** handle alignment properly

            block allocated_block = m_sub_alloc.allocate(requested_size + sizeof(block), requested_alignment);
            // create a copy of the block structure at the allocated block
            new(allocated_block.base) block(allocated_block);
            // return a pointer to the allocated memory, leaving the block structure just behind.
            return allocated_block.base + sizeof(block);
        }

        // assumed pointer is valid. If it causes segmentation fault, blame whoever gave it.
        void deallocate(std::byte * memory)
        {
            if (owns(memory))
            {
                // get reference to the block structure
                auto &allocated_block = *reinterpret_cast<block *>(memory - sizeof(block));

                m_sub_alloc.deallocate(allocated_block);
                allocated_block.~block();
            }
        }

        void deallocate_all() { m_sub_alloc.deallocate_all(); }

        // assumed pointer is advanced, as returned on allocation
        bool owns(const std::byte * memory) const
        { return m_sub_alloc.owns(memory - sizeof(block)); }

        bool owns(const block &memory) const
        { return m_sub_alloc.owns(memory); }
    };

}
