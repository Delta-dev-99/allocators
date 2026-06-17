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
        // 
        // TODO: review and test last changes
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
            memory::block pop_slice(std::size_t requested_size, std::size_t requested_alignment = 1)
            {
                // This function searches for a block that satisfies the request and complies with the rules of the allocator.
                // considering that block headers also have alignment requirements,
                // we need to find a block that 
                // we need to find a block: [header alignment padding | header | alignment padding | returned block ]
                // we also need to ensure that a block header can be placed on the returned block (to allow deallocation)

                // smaller allocations not allowed
                if (requested_size < sizeof(Block_Header)) requested_size = sizeof(Block_Header);
                // less alignment not allowed
                if (requested_alignment < alignof(Block_Header)) requested_alignment = alignof(Block_Header);

                auto current = first;
                decltype(current) prev = nullptr;
                while (current)
                {
                    auto current_block = current->get_memory_block();
                    auto current_aligned = align_up(current_block.base, requested_alignment);

                    // ensure any space left before the block is enough for a block header
                    if (current_aligned != current_block.base)
                    {
                        while(static_cast<std::size_t>(current_aligned - current_block.base) < sizeof(Block_Header))
                        {
                            current_aligned = align_up(current_aligned + 1, requested_alignment);
                        }
                        // note: haven't decided whether the block is suitable yet, so we can't touch the headers.
                    }
                    
                    // size after aligning-up the start of the block
                    auto aligned_size = static_cast<std::size_t>(current_block.get_end() - current_aligned);

                    if (aligned_size >= requested_size)
                    {
                        // found a suitable block.
                        // we will be allocating from this one.

                        // trim space at the start if necessary, destroy header otherwise
                        if (current_aligned != current_block.base)
                        {
                            // trimming the start. we just leave the header there and update the size
                            current->size -= static_cast<std::size_t>(current_aligned - current_block.base);

                            // current now points to the block before the one we are allocating
                        }
                        else
                        {
                            // not trimming at the start.
                            // we need to destroy the block header present there.
                            if (prev) prev->next = current->next;
                            else first = current->next;
                            current->~Block_Header();

                            // update current so that it points to the block before the one we are allocating
                            if (prev) current = prev;
                            else current = nullptr;
                        }

                        // note: the block, starting at current_aligned, is now not in the linked list

                        // end of the block aligned-up for next block header
                        auto aligned_end = align_up(current_aligned + requested_size, alignof(Block_Header));
                        // size left after aligned_end
                        auto aligned_oversize = static_cast<std::size_t>(current_block.get_end() - aligned_end);

                        // check whether we can trim some space at the end
                        if (aligned_oversize >= sizeof(Block_Header))
                        {
                            // Remaining space is large enough to fit a free list header.
                            // Divide the block.

                            // we won't need coalescing because if it were possible it would have been done already.
                            // we are just putting back the end of the block.
                            // insert between current and the next one.
                            Block_Header * new_header;
                            if (current) // current may be nullptr if we allocated from the very first block without trimming the start
                            {
                                new_header = new (aligned_end) Block_Header{.next = current->next, .size = aligned_oversize};
                                current->next = new_header;
                            }
                            else
                            {
                                new_header = new (aligned_end) Block_Header{.next = first, .size = aligned_oversize};
                                first = new_header;
                            }

                            return block{.base = current_aligned, .size = static_cast<std::size_t>(aligned_end - current_aligned)};
                        }
                        else
                        {
                            // Can't leave a space smaller than sizeof(Block_Header).
                            // Extract the full block

                            // not much to do because the block is already not in the freelist
                            return block{.base = current_aligned, .size = aligned_size};
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
            return m_free_list.pop_slice(requested_size, requested_alignment);
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
