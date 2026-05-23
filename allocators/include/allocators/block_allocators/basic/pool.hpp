#pragma once

#include <allocators/block_allocators/block_allocator.hpp>
#include <allocators/block_allocators/internal/structures/free_list.hpp>


namespace dd99::memory::block_allocator
{
    // Allocate fixed-size blocks
    // Uses a free list (forward list with nodes in unused blocks)
    // Requires Block_Size to be large enough to fit a list node in a block
    template <std::size_t Block_Size,
              std::size_t Block_Alignment = Block_Size>
    class Pool
    {
        // Note, block size is statically asserted on the freelist class
        static_assert(Block_Alignment > 0,
            "Block_Alignment must be greater than 0");
        static_assert((Block_Alignment & (Block_Alignment - 1)) == 0,
            "Block_Alignment must be a power of 2");
        static_assert(Block_Size % Block_Alignment == 0,
            "Block_Size must be a multiple of Block_Alignment; "
            "this ensures every block index preserves the alignment guarantee");

    public:
        // Expects: provided memory to be aligned to at least Block_Alignment
        Pool(const memory::Block &memory)
            : m_memory(memory)
            , m_free_list()
        {
            // TODO: assert memory is aligned
            build_free_list();
        }

    public:
        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size, std::size_t requested_alignment = Block_Alignment)
        {
            static_cast<void>(requested_alignment); // all blocks are aligned
            // TODO: assert (requested_alignment <= Block_Alignment) && (requested_alignment & (requested_alignment - 1) == 0)

            // larger allocations not supported
            if ((requested_size > Block_Size) || m_free_list.empty())
                return {};
            
            return m_free_list.pop();
        }

        void deallocate(const memory::Block &memory)
        {
            if (owns(memory))
                m_free_list.push(memory);
        }

        void deallocate_all()
        {
            m_free_list.clear();
            build_free_list();
        }

        bool owns(const std::byte * memory) const
        {
            return m_memory.contains(memory);
        }

        bool owns(const memory::Block& memory) const
        {
            return m_memory.contains(memory);
        }

    private:
        void build_free_list()
        {
            memory::Block current{.base = m_memory.base, .size = Block_Size};
            while(current.get_end() <= m_memory.get_end())
            {
                m_free_list.push(current);
                // advance
                current.base = current.base + Block_Size;
            }
        }

    private:
        memory::Block m_memory;
        memory::structure::Freelist<Block_Size> m_free_list;
    };

    static_assert(Block_Allocator<Pool<sizeof(void*)>>, "This definition doesn't comply with the `Block_Allocator` concept");


}
