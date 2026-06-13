#pragma once

#include <allocators/block_allocators/block_allocator.hpp>
#include <allocators/structures/forward_list.hpp>
#include <allocators/alignment.hpp>


namespace dd99::memory::block_allocator
{

    namespace detail
    {
        // Singly linked list of memory blocks that stores
        // nodes on blocks and allows different block sizes.
        // This is a custom freelist that keeps the nodes sorted,
        // joins adjacent free blocks and slices blocks as needed.
        class Slicing_Freelist
        {
        protected:
            // header for free blocks
            // the nodes of the list
            struct Block_Header
            {
                Block_Header *next = nullptr;
                std::size_t size;

                // get corresponding memory block
                memory::block get_memory_block()
                { return {.base = reinterpret_cast<std::byte *>(this), .size = size}; }
            };
            static constexpr auto Block_Header_Alignment = alignof(Block_Header);

        public:
            ~Slicing_Freelist()
            {
                // call destructor on created nodes
                clear();
            }

        public:
            // get a memory slice with at least the specified size
            memory::block pop_slice(std::size_t min_size, std::size_t requested_alignment = 1)
            {
                // This function searches for a block that satisfies the request and complies with the rules of the allocator.
                // considering that block headers also have alignment requirements,
                // we need to find a block that 
                // we need to find a block: [header alignment padding | header | alignment padding | returned block ]
                // we also need to ensure that a block header can be placed on the returned block (to allow deallocation)

                // smaller allocations not allowed
                if (min_size < sizeof(Block_Header))
                    min_size = sizeof(Block_Header);

                auto current = first;
                decltype(current) prev = nullptr;
                while (current)
                {
                    auto current_block = current->get_memory_block();
                    auto current_aligned = align_up(current_block.base, requested_alignment);
                    

                    if (current->size >= min_size)
                    {
                        // found a suitable block.
                        if (current->size >= sizeof(Block_Header) + min_size)
                        {
                            // Remaining space is large enough to fit a free list header.
                            // Divide the block.
                            current->size -= min_size;
                            return {.base = reinterpret_cast<std::byte *>(current) + current->size, .size = min_size};
                        }
                        else
                        {
                            // Can't leave a space smaller than sizeof(Block_Header).
                            // Extract the full block
                            if (prev) prev->next = current->next;
                            else first = current->next;
                            
                            auto r = current->get_memory_block();
                            current->~Block_Header();
                            return r;
                        }
                    }

                    prev = current;
                    current = current->next;
                }

                // no suitable memory found
                return {};
            }

            // sorted by base address
            void push(const memory::block& block)
            {
                // insert into list and get pointer to node previous to inserted one
                // the pointer to the inserted node is do_insert(memory)->next
                auto prev_ptr = do_insert(block);

                // Merge adjacent free blocks
                // try merging with previous first
                if (prev_ptr)
                {
                    if (!try_merge(prev_ptr))
                        prev_ptr = prev_ptr->next;
                }
                else
                    prev_ptr = first;

                // try merge with next
                try_merge(prev_ptr);
            }

            void clear()
            {
                auto current = first;
                first = nullptr;
                while (current)
                {
                    const auto tmp = current->next;
                    current->~Block_Header();
                    current = tmp;
                }
            }

            bool empty() const
            {
                return first == nullptr;
            }
        
        private:
            // returns a pointer to the node before the inserted node
            Block_Header *do_insert(const memory::block& memory)
            {
                //
                // find position and link new node
                //

                if (!first)
                {
                    auto new_header = new (memory.base) Block_Header{.next = nullptr, .size = memory.size};
                    first = new_header;
                    return nullptr;
                }

                if (reinterpret_cast<std::byte *>(first) > memory.base)
                {
                    auto new_header = new (memory.base) Block_Header{.next = first, .size = memory.size};
                    first = new_header;
                    return nullptr; // node before the new node does not exist
                }

                // NOTE: PROFILING: This loop is a performance bottleneck
                auto place = first;
                while(place->next != nullptr)
                {
                    if (reinterpret_cast<std::byte *>(place->next) > memory.base)
                    {
                        auto new_header = new (memory.base) Block_Header{.next = place->next, .size = memory.size};
                        place->next = new_header;
                        return place;
                    }
                    place = place->next;
                }

                auto new_header = new (memory.base) Block_Header{.size = memory.size};
                place->next = new_header;
                return place;
            }

            // merge a block with the next one if adjacent
            bool try_merge(Block_Header *prev_ptr)
            {
                if (prev_ptr->get_memory_block().get_end() == reinterpret_cast<std::byte *>(prev_ptr->next))
                {
                    auto to_merge_ptr = prev_ptr->next;
                    prev_ptr->size += to_merge_ptr->size;
                    prev_ptr->next = to_merge_ptr->next;
                    to_merge_ptr->~Block_Header();
                    return true;
                }

                return false;
            }
        
        private:
            Block_Header *first = nullptr;
        };
    
    }

    // Allocates memory blocks by dividing a large enough free memory block (chopping the requested size).
    // Uses a free list (linked list with nodes in unused blocks)
    // Requires allocation sizes to be large enough to fit a list node (adjusted if not)
    // TODO: This is unfinished
    class Slicing
    {
    public:
        Slicing(const memory::block& memory)
            : m_memory(memory)
            , m_free_list()
        {
            m_free_list.push(m_memory);
        }

    public:
        [[nodiscard]]
        memory::block allocate(std::size_t requested_size, std::size_t requested_alignment = 1)
        {
            return m_free_list.pop_slice(requested_size);
        }

        void deallocate(const memory::block &memory)
        {
            if (owns(memory))
                m_free_list.push(memory);
        }

        void deallocate_all()
        {
            m_free_list.clear();
            m_free_list.push(m_memory);
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
        memory::block m_memory;
        detail::Slicing_Freelist m_free_list;
    };

    static_assert(Block_Allocator<Slicing>, "This definition doesn't comply with the `Block_Allocator` concept");
    

}
