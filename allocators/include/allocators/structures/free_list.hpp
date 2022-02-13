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
        // NOTE: Undefined Behaviour if the freelist is empty. Do check!
        [[nodiscard]]
        memory::Block pop()
        {
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



    // doubly linked list of free memory blocks that stores
    // nodes on empty blocks
    class Freelist_Double_Link
    {
    protected:
        // Header for free blocks. The nodes of the freelist
        struct Free_Block_Header { Free_Block_Header *next = nullptr; Free_Block_Header *prev = nullptr; };

    private:
        std::size_t m_block_size;
        Free_Block_Header *first = nullptr;

    public:
        Freelist_Double_Link(std::size_t block_size)
            : m_block_size(block_size)
        {
            if (sizeof(Free_Block_Header) > block_size)
                throw std::length_error{"Freelist_Double_Link: block_size (" + std::to_string(block_size) + ") is too short. Min: " + std::to_string(sizeof(Free_Block_Header))};
        }

        Freelist_Double_Link(const Freelist_Double_Link &) = delete;
        Freelist_Double_Link(Freelist_Double_Link &&other) = default;

        ~Freelist_Double_Link()
        {
            // call destructor on created nodes
            clear();
        }


    public:
        // NOTE: Undefined Behaviour if the freelist is empty. Do check!
        [[nodiscard]]
        memory::Block pop()
        {
            auto block_ptr = first;
            first = first->next;
            first->prev = nullptr;
            block_ptr->~Free_Block_Header();
            return {.base = block_ptr, .size = m_block_size};
        }

        void push(const memory::Block& block)
        {
            auto new_header = new (block.base) Free_Block_Header{.next = first};
            first->prev = new_header;
            first = new_header;
        }

        void remove(const memory::Block & block)
        {
            // NOTE: Assumed block is in the freelist
            // UB otherwise.

            auto header = reinterpret_cast<Free_Block_Header *>(block.base);
            header->prev->next = header->next;
            header->~Free_Block_Header();
        }

        void clear()
        {
            // NOTE: If the destruction of the block header is trivial,
            // this function reduces to setting `first` to `nullptr`
            // NOTE: I know destruction IS trivial, but compiler should know too.

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
