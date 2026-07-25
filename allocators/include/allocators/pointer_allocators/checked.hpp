#pragma once

#include <allocators/pointer_allocators/allocator.hpp>
#include <allocators/exception.hpp>
#include <bit>

namespace dd99_allocators_namespace::pointer_allocator
{
    // A variant that adds a small check to avoid errors with pointers
    // Allocated block layout:
    // ----------------------------------------
    // | block | Checksum | memory | Checksum |
    // ----------------------------------------
    // The returned pointer points to the start of `memory`.
    template <class Sub_Alloc_T>
    class Pointer_Checked
    {
    public:
        Pointer_Checked(Sub_Alloc_T sub_allocator)
            : m_sub_alloc(std::forward<Sub_Alloc_T>(sub_allocator))
        { }

    public:
        Sub_Alloc_T m_sub_alloc;

    private:
        using Check_Header = int;
    
    public:
        [[nodiscard]]
        std::byte * allocate(std::size_t requested_size)
        {
            auto const alloc_size = requested_size + sizeof(block) + 2 * sizeof(Check_Header);
            block allocated_block = m_sub_alloc.allocate(alloc_size);
            // create a copy of the block structure at the allocated block
            new (allocated_block.base) block(allocated_block);
            // compute check header and copy to both locations
            auto check_header = compute_check(allocated_block);
            new (get_start_check_ptr(allocated_block)) Check_Header(check_header);
            new (get_end_check_ptr(allocated_block)) Check_Header(check_header);
            // return a pointer to the allocated memory, leaving the block structure just behind.
            return memory_block_to_ptr(allocated_block);
        }

        // assumed pointer is valid. If it causes segmentation fault, blame whoever gave it.
        void deallocate(std::byte *memory)
        {
            if (owns(memory))
            {
                // get reference to the block structure
                auto &allocated_block = ptr_to_memory_block_ref(memory);
                
                auto check = compute_check(allocated_block);
                if ((check != *get_start_check_ptr(allocated_block)) || (check != *get_end_check_ptr(allocated_block)))
                {
                    throw dd99_allocators_namespace::memory_corrupted{};
                }

                // probably not necesary to make a copy
                auto allocated_block_cpy = allocated_block;
                allocated_block.~block();
                m_sub_alloc.deallocate(allocated_block_cpy);
            }
        }

        void deallocate_all() { m_sub_alloc.deallocate_all(); }

        bool owns(const std::byte * memory) const
        { return m_sub_alloc.owns(memory); }

        bool owns(const block &memory) const
        { return m_sub_alloc.owns(memory); }

    private:
        static Check_Header * get_start_check_ptr(const block &memory)
        {
            return reinterpret_cast<Check_Header *>(memory.base + sizeof(block));
        }

        static Check_Header * get_end_check_ptr(const block &memory)
        {
            return reinterpret_cast<Check_Header *>(memory.get_end() - sizeof(Check_Header));
        }

        static Check_Header compute_check(const block &memory)
        {
            // hash the memory block structure
            // anything should do
            return Check_Header(std::rotl(reinterpret_cast<std::uintptr_t>(memory.base), 15) ^ memory.size);
        }

        static std::byte * memory_block_to_ptr(const block &memory)
        {
            return memory.base + sizeof(block) + sizeof(Check_Header);
        }

        static block &ptr_to_memory_block_ref(std::byte *memory)
        {
            return *reinterpret_cast<block *>(memory - sizeof(block) - sizeof(Check_Header));
        }
    };
}
