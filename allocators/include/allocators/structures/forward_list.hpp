#pragma once

#include <cstddef>



namespace dd99::memory::structure
{

    // doesn't store any elements (not a container).
    // the important data is the memory address of the nodes.
    struct basic_forward_list
    {
        struct node { node * m_next_ptr = nullptr; };
        
        constexpr basic_forward_list() = default;
        constexpr ~basic_forward_list() { clear(); }

        constexpr basic_forward_list(const basic_forward_list &) = delete;
        constexpr basic_forward_list(basic_forward_list && other)
            : m_first_ptr{other.m_first_ptr}
        {
            other.m_first_ptr = nullptr;
        }

        constexpr basic_forward_list & operator=(const basic_forward_list &) = delete;
        constexpr basic_forward_list & operator=(basic_forward_list && other)
        {
            clear();
            m_first_ptr = other.m_first_ptr;
            other.m_first_ptr = nullptr;
            return * this;
        }

        // returns the address of the removed node
        // precondition: list not empty
        [[nodiscard]]
        constexpr
        std::byte *
        pop()
        {
            // TODO: assert(!empty())

            const auto current = m_first_ptr;
            std::byte * node_address = reinterpret_cast<std::byte *>(current);

            m_first_ptr = current->m_next_ptr;
            current->~node();

            return node_address;
        }

        // precondition: node_address is suitably aligned
        // precondition: memory range (node_address, sizeof(node)) can be used to store a node
        constexpr
        void
        push(std::byte * node_address)
        {
            // TODO: assert alignment of the given pointer
            node * new_node_ptr = new (node_address) node{.m_next_ptr = m_first_ptr};
            m_first_ptr = new_node_ptr;
        }

        constexpr
        void
        clear()
        {
            auto current_ptr = m_first_ptr;
            m_first_ptr = nullptr;
            while (current_ptr != nullptr)
            {
                const auto to_be_destroyed_ptr = current_ptr;
                current_ptr = current_ptr->m_next_ptr;
                to_be_destroyed_ptr->~node();
            }
        }

        [[nodiscard]]
        constexpr
        bool
        empty() const
        { return m_first_ptr == nullptr; }



        node * m_first_ptr = nullptr;
    };

}
