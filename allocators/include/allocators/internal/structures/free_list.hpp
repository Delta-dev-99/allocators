#pragma once

#include <allocators/exception.hpp>
#include <cstddef>
#include <new>
// #include <stdexcept>
// #include <string>


namespace dd99::memory::structure
{
    // singly linked list of free memory blocks that stores
    // nodes on empty blocks
    template <std::size_t Block_Size>
    class Freelist
    {
    protected:
        // Header for free blocks. The nodes of the freelist
        struct Free_Block_Header { Free_Block_Header *next = nullptr; };
        static_assert(sizeof(Free_Block_Header) <= Block_Size, "freelist block size is too small to store the freelist node header");

    private:
        Free_Block_Header *first = nullptr;

    public:
        ~Freelist()
        {
            // call destructor on created nodes
            clear();
        }

        Freelist()
        {
        }

        Freelist(const Freelist &) = delete;
        Freelist(Freelist &&other) = default;

        Freelist & operator=(const Freelist &) = delete;
        Freelist & operator=(Freelist &&) = default;


    public:
        // NOTE: Undefined Behaviour if the freelist is empty. Do check!
        [[nodiscard]]
        memory::Block pop()
        {
            auto current = first;
            first = first->next;
            current->~Free_Block_Header();
            return {.base = reinterpret_cast<std::byte *>(current), .size = Block_Size};
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
    template <std::size_t Block_Size>
    class Freelist_Double_Link
    {
    protected:
        // Header for free blocks. The nodes of the freelist
        struct Free_Block_Header { Free_Block_Header *next = nullptr; Free_Block_Header *prev = nullptr; };
        static_assert(sizeof(Free_Block_Header) <= Block_Size, "freelist block size is too small to store the freelist node header");

    private:
        Free_Block_Header *m_first = nullptr;

    public:
        ~Freelist_Double_Link()
        {
            // call destructor on created nodes
            clear();
        }

        Freelist_Double_Link()
        {
        }

        Freelist_Double_Link(const Freelist_Double_Link &) = delete;
        Freelist_Double_Link(Freelist_Double_Link && other)
            : m_first(other.m_first)
        {
            other.m_first = nullptr;
        }

        Freelist_Double_Link & operator=(const Freelist_Double_Link &) = delete;
        Freelist_Double_Link & operator=(Freelist_Double_Link && other)
        {
            clear();
            m_first = other.m_first;
            return *this;
        }

    public:
        // NOTE: Undefined Behaviour if the freelist is empty. Do check!
        [[nodiscard]]
        memory::Block pop()
        {
            auto block_ptr = m_first;
            const memory::Block block{.base = reinterpret_cast<std::byte *>(block_ptr), .size = Block_Size};
            remove(block);
            return block;
        }

        void push(const memory::Block& block)
        {
            auto new_header = new (block.base) Free_Block_Header{.next = m_first};

            if (m_first)
                m_first->prev = new_header;

            m_first = new_header;
        }

        void remove(const memory::Block & block)
        {
            // NOTE: Assumed block is in the freelist
            // UB otherwise.

            auto header = reinterpret_cast<Free_Block_Header *>(block.base);

            if (header->prev == nullptr) // removing the first block
            {
                m_first = header->next;
            }
            else
            {
                header->prev->next = header->next;
            }

            if (header->next)
                header->next->prev = header->prev;

            header->~Free_Block_Header();
        }

        void clear()
        {
            // NOTE: If the destruction of the block header is trivial,
            // this function reduces to setting `m_first` to `nullptr`
            // NOTE: I know destruction IS trivial, but compiler should know too.

            auto current = m_first;
            m_first = nullptr;
            while (current)
            {
                const auto tmp = current->next;
                current->~Free_Block_Header();
                current = tmp;
            }
        }

        bool empty() const
        {
            return m_first == nullptr;
        }

    };

}
