#pragma once

#include <allocators/pointer/allocator.hpp>

namespace dd99::memory::pointer_allocator
{
    // A variant that adds a small check to avoid errors with pointers
    template <class Sub_Alloc_T>
    class Pointer_Checked : public Allocator
    {
    public:
        Pointer_Checked(Sub_Alloc_T&& sub_allocator)
            : m_sub_alloc(std::move(sub_allocator))
        { }

    protected:
        Sub_Alloc_T m_sub_alloc;

    private:
        using Check_Header = int;
    
    public:
        [[nodiscard]]
        void *allocate(std::size_t requested_size)
        {
            auto const alloc_size = requested_size + sizeof(Block) + 2 * sizeof(Check_Header);
            Block allocated_block = m_sub_alloc.allocate(alloc_size);
            // create a copy of the Block structure at the allocated block
            new (allocated_block.base) Block(allocated_block);
            // compute check header and copy to both locations
            auto check_header = compute_check(allocated_block);
            new (get_start_check_ptr(allocated_block)) Check_Header(check_header);
            new (get_end_check_ptr(allocated_block)) Check_Header(check_header);
            // return a pointer to the allocated memory, leaving the Block structure just behind.
            return memory_block_to_ptr(allocated_block);
        }

        // assumed pointer is valid. If it causes segmentation fault, blame whoever gave it.
        void deallocate(void *memory)
        {
            if (owns(memory))
            {
                // get reference to the Block structure
                auto &allocated_block = ptr_to_memory_block_ref(memory);
                
                auto check = compute_check(allocated_block);
                if ((check != *get_start_check_ptr(allocated_block)) || (check != *get_end_check_ptr(allocated_block)))
                {
                    // TODO: Throw some apropiate exception type
                    throw "Memory check on deallocation failed! Overriden not owned memory!";
                }

                // probably not necesary to make a copy
                auto allocated_block_cpy = allocated_block;
                allocated_block.~Block();
                m_sub_alloc.deallocate(allocated_block_cpy);
            }
        }

        void deallocate_all() { m_sub_alloc.deallocate_all(); }

        bool owns(void *memory) const
        { return m_sub_alloc.owns(memory); }

        bool owns(const Block &memory) const
        { return m_sub_alloc.owns(memory); }

    private:
        static Check_Header *get_start_check_ptr(const Block &memory)
        {
            return reinterpret_cast<Check_Header *>
                (reinterpret_cast<std::uintptr_t>(memory.base) + sizeof(Block));
        }

        static Check_Header *get_end_check_ptr(const Block &memory)
        {
            return reinterpret_cast<Check_Header *>
                (reinterpret_cast<std::uintptr_t>(memory.base) + memory.size - sizeof(Check_Header));
        }

        static Check_Header compute_check(const Block &memory)
        {
            // anything should do
            return reinterpret_cast<std::uintptr_t>(memory.base) ^ memory.size;
        }

        static void *memory_block_to_ptr(const Block &memory)
        {
            return reinterpret_cast<void *>
                (reinterpret_cast<std::uintptr_t>(memory.base) + sizeof(Block) + sizeof(Check_Header));
        }

        static Block &ptr_to_memory_block_ref(void *memory)
        {
            return *reinterpret_cast<Block *>
                (reinterpret_cast<std::uintptr_t>(memory) - sizeof(Block) - sizeof(Check_Header));
        }
    };
}
