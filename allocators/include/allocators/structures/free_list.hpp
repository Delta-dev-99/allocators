#pragma once

#include <cstddef>
#include <new>
#include <stdexcept>
#include <string>


namespace dd99::memory::structure
{
    // singly linked list of free memory blocks that stores
    // nodes on empty blocks
    class Freelist
    {
    protected:
        // Header for free blocks. The nodes of the freelist
        struct Free_Block_Header { Free_Block_Header *next = nullptr; };

    private:
        std::size_t m_block_size;
        Free_Block_Header *first = nullptr;

    public:
        Freelist(std::size_t block_size)
            : m_block_size(block_size)
        {
            if (sizeof(Free_Block_Header) > block_size)
                throw std::length_error{"Freelist: block_size (" + std::to_string(block_size) + ") is too short. Min: " + std::to_string(sizeof(Free_Block_Header))};
        }

        Freelist(const Freelist &) = delete;
        Freelist(Freelist &&other) = default;

        ~Freelist()
        {
            // call destructor on created nodes
            clear();
        }


    public:
        [[nodiscard]]
        memory::Block pop()
        {
            if (!first) return {};
            
            auto current = first;
            first = first->next;
            current->~Free_Block_Header();
            return {.base = current, .size = m_block_size};
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

        bool empty() const
        {
            return first == nullptr;
        }

    };

}
