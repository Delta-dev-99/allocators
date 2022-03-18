#pragma once

#include <allocators/internal/bases/pointer.hpp>

namespace dd99::memory::pointer_allocator
{
    // Allows the allocation and deallocation of memory using pointers
    // This allocator is the bridge between this library and standard c++ allocation
    // NOTE that this is not a composable allocator,
    // nor compatible with the rest of allocators on this library.
    template <class Sub_Alloc_T>
    class Pointer : public Allocator
    {
    public:
        Pointer(Sub_Alloc_T&& sub_allocator)
            : m_sub_alloc(std::move(sub_allocator))
        { }

    protected:
        Sub_Alloc_T m_sub_alloc;
    
    public:
        [[nodiscard]]
        std::byte *allocate(std::size_t requested_size)
        {
            Block allocated_block = m_sub_alloc.allocate(requested_size + sizeof(Block));
            // create a copy of the Block structure at the allocated block
            new(allocated_block.base) Block(allocated_block);
            // return a pointer to the allocated memory, leaving the Block structure just behind.
            return allocated_block.base + sizeof(Block);
        }

        // assumed pointer is valid. If it causes segmentation fault, blame whoever gave it.
        void deallocate(std::byte *memory)
        {
            if (owns(memory))
            {
                // get reference to the Block structure
                auto &allocated_block = *reinterpret_cast<Block *>(memory - sizeof(Block));

                m_sub_alloc.deallocate(allocated_block);
                allocated_block.~Block();
            }
        }

        void deallocate_all() { m_sub_alloc.deallocate_all(); }

        // assumed pointer is advanced, as returned on allocation
        bool owns(std::byte *memory) const
        { return m_sub_alloc.owns(memory - sizeof(Block)); }

        bool owns(const Block &memory) const
        { return m_sub_alloc.owns(memory); }
    };

}
