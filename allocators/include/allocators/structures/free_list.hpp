#pragma once

#include <cstddef>
#include <new>


namespace dd99::memory::structures
{
    // singly linked list of free memory blocks that stores
    // nodes on empty blocks
    template <std::size_t Block_Size>
    class Freelist_Fixed_Sz_Blocks
    {
    protected:
        // Header for free blocks. The nodes of the freelist
        struct Free_Block_Header { Free_Block_Header *next = nullptr; };
        static_assert(sizeof(Free_Block_Header) <= Block_Size);

    private:
        Free_Block_Header *first = nullptr;

    public:
        Freelist_Fixed_Sz_Blocks() = default;
        Freelist_Fixed_Sz_Blocks(const Freelist_Fixed_Sz_Blocks &) = delete;
        Freelist_Fixed_Sz_Blocks(Freelist_Fixed_Sz_Blocks &&other) = default;

        ~Freelist_Fixed_Sz_Blocks()
        {
            // call destructor on created nodes
            clear();
        }

        memory::Block pop()
        {
            if (!first) return {};
            auto current = first;
            first = first->next;
            current->~Free_Block_Header();
            return {.base = current, .size = Block_Size};
        }

        void push(const memory::Block& memory)
        {
            auto new_header = new (memory.base) Free_Block_Header{.next = first};
            first = new_header;
        }

        void clear()
        {
            auto current = first;
            first = nullptr;
            while (current)
            {
                const auto tmp = current->next;
                current->~Free_Block_Header();
                current = tmp;
            }
        }
    };


    // singly linked list of free memory blocks that stores
    // nodes on empty blocks of different sizes
    class Freelist_Sized_Blocks
    {
    protected:
        // header for free blocks
        // the nodes of the list
        struct Free_Sized_Block_Header
        {
            Free_Sized_Block_Header *next = nullptr;
            std::size_t size;

            // get corresponding memory block
            auto get_memory_block() { return memory::Block{.base = this, .size = size}; }
        };
        
    private:
        Free_Sized_Block_Header *first = nullptr;

    public:
        Freelist_Sized_Blocks() = default;
        Freelist_Sized_Blocks(const Freelist_Sized_Blocks &) = delete;
        Freelist_Sized_Blocks(Freelist_Sized_Blocks &&other) = default;

        ~Freelist_Sized_Blocks()
        {
            // call destructor on created nodes
            clear();
        }

        memory::Block extract(std::size_t min_size)
        {
            // smaller allocations not allowed
            if (min_size < sizeof(Free_Sized_Block_Header))
                min_size = sizeof(Free_Sized_Block_Header);

            auto current = first;
            decltype(current) prev = nullptr;
            while (current)
            {
                if (current->size >= min_size)
                {
                    // found a suitable block.
                    if (current->size - min_size >= sizeof(Free_Sized_Block_Header))
                    {
                        // Remaining space is large enough to fit a free list header.
                        // Divide the block.
                        current->size -= min_size;
                        return {.base = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(current) + current->size), .size = min_size};
                    }
                    else
                    {
                        // Can't leave a space smaller than sizeof(Free_Sized_Block_Header).
                        // Extract the full block
                        if (prev) prev->next = current->next;
                        else first = current->next;
                        
                        auto r = current->get_memory_block();
                        current->~Free_Sized_Block_Header();
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
        void insert(const memory::Block& memory)
        {
            // insert into list and get pointer to node previous to inserted one
            // the pointer to the inserted node is do_insert(memory)->next
            auto prev_ptr = do_insert(memory);

            // Merge adjacent free blocks
            if (prev_ptr)
            {
                if (!try_merge(prev_ptr))
                    prev_ptr = prev_ptr->next;
            }
            else
                prev_ptr = first;

            try_merge(prev_ptr);
        }

        void clear()
        {
            auto current = first;
            first = nullptr;
            while (current)
            {
                const auto tmp = current->next;
                current->~Free_Sized_Block_Header();
                current = tmp;
            }
        }
    
    private:
        // returns a pointer to the node before the inserted node
        Free_Sized_Block_Header *do_insert(const memory::Block& memory)
        {
            //
            // find position and link new node
            //

            if (!first)
            {
                auto new_header = new (memory.base) Free_Sized_Block_Header{.next = nullptr, .size = memory.size};
                first = new_header;
                return nullptr;
            }

            if (first > memory.base)
            {
                auto new_header = new (memory.base) Free_Sized_Block_Header{.next = first, .size = memory.size};
                first = new_header;
                return nullptr; // node before the new node does not exist
            }

            auto place = first;
            while(place->next)
            {
                if (place->next > memory.base)
                {
                    auto new_header = new (memory.base) Free_Sized_Block_Header{.next = place->next, .size = memory.size};
                    place->next = new_header;
                    return place;
                }
                place = place->next;
            }

            auto new_header = new (memory.base) Free_Sized_Block_Header{.size = memory.size};
            place->next = new_header;
            return place;
        }

        // merge a block with the next one if adjacent
        bool try_merge(Free_Sized_Block_Header *prev_ptr)
        {
            if (prev_ptr->get_memory_block().get_end() == prev_ptr->next)
            {
                prev_ptr->size += prev_ptr->next->size;
                prev_ptr->next = prev_ptr->next->next;
                prev_ptr->next->~Free_Sized_Block_Header();
                return true;
            }

            return false;
        }
    };
}
