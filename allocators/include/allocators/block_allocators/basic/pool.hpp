#pragma once

#include <allocators/block_allocators/block_allocator.hpp>
#include <allocators/structures/forward_list.hpp>


namespace dd99::memory::block_allocator
{
    // Allocate fixed-size blocks
    // Uses a free list (forward list with nodes in unused blocks)
    // Requires Block_Size to be large enough to fit a list node in a block
    template <std::size_t Block_Size,
              std::size_t Block_Alignment = Block_Size>
    class Pool
    {
        using freelist_type = memory::structure::basic_forward_list;

        static constexpr auto block_size = Block_Size;
        static constexpr auto block_alignment = Block_Alignment;

        // Note, block size is statically asserted on the freelist class
        static_assert(block_alignment > 0,
            "Block_Alignment must be greater than 0");
        static_assert((block_alignment & (block_alignment - 1)) == 0,
            "Block_Alignment must be a power of 2");
        static_assert(block_size % block_alignment == 0,
            "Block_Size must be a multiple of Block_Alignment; "
            "this ensures every block index preserves the alignment guarantee");
        static_assert(block_size >= sizeof(freelist_type::node),
            "Block_Size must be large enough to fit a freelist node");
        static_assert(block_alignment >= alignof(freelist_type::node),
            "Block_Alignment must be appropriate for freelist nodes");

    public:
        // Expects: provided memory to be aligned to at least Block_Alignment
        Pool(const memory::block &memory)
            : m_memory(memory)
        {
            // TODO: assert memory is aligned
            build_free_list();
        }

    public:
        [[nodiscard]]
        memory::block allocate(std::size_t requested_size, std::size_t requested_alignment = block_alignment)
        {
            static_cast<void>(requested_alignment); // all blocks are aligned
            // TODO: assert (requested_alignment <= Block_Alignment) && (requested_alignment & (requested_alignment - 1) == 0)

            // larger allocations not supported
            if ((requested_size > block_size) || m_freelist.empty())
                return {};
            
            return block{.base = m_freelist.pop(), .size = block_size};
        }

        void deallocate(const memory::block &memory)
        {
            if (owns(memory))
                m_freelist.push(memory.base);
        }

        void deallocate_all()
        {
            m_freelist.clear();
            build_free_list();
        }

        bool owns(const std::byte * memory) const
        {
            return m_memory.contains(memory);
        }

        bool owns(const memory::block& memory) const
        {
            return m_memory.contains(memory);
        }

    private:
        void build_free_list()
        {
            memory::block current{.base = m_memory.base, .size = block_size};
            while(current.get_end() <= m_memory.get_end())
            {
                m_freelist.push(current.base);
                // advance
                current.base += block_size;
            }
        }

    private:
        memory::block m_memory;
        freelist_type m_freelist;
    };

    static_assert(Block_Allocator<Pool<sizeof(void*)>>, "This definition doesn't comply with the `Block_Allocator` concept");


}
