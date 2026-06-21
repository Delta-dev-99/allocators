#pragma once

#include <allocators/library_configuration/cpp_config.hpp>
#include <allocators/alignment.hpp>
#include <cstddef>
#include <new>



namespace dd99::memory::structure
{

    // doesn't store any elements (not a container).
    // the important data is the memory address of the nodes.
    struct basic_linked_list
    {
        struct node { node * m_next_ptr = nullptr; node * m_prev_ptr = nullptr; };
        
        constexpr basic_linked_list() = default;
        constexpr ~basic_linked_list() { clear(); }

        constexpr basic_linked_list(const basic_linked_list &) = delete;
        constexpr basic_linked_list(basic_linked_list && other)
            : m_first_ptr{other.m_first_ptr}
        {
            other.m_first_ptr = nullptr;
        }

        constexpr basic_linked_list & operator=(const basic_linked_list &) = delete;
        constexpr basic_linked_list & operator=(basic_linked_list && other)
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
            DD99_ALLOCATORS_ASSERT_HARDENED("tried to pop() empty list", !empty());

            const auto current = m_first_ptr;
            std::byte * node_address = reinterpret_cast<std::byte *>(current);

            m_first_ptr = current->m_next_ptr;
            if (m_first_ptr) m_first_ptr->m_prev_ptr = nullptr;
            current->~node();

            return node_address;
        }

        // create a node at the specified address, and link it
        constexpr
        void
        push(std::byte * node_address)
        {
            DD99_ALLOCATORS_ASSERT_HARDENED("tried to push unaligned pointer", is_aligned(node_address, alignof(node)));

            node * new_node_ptr = new (node_address) node{.m_next_ptr = m_first_ptr}; // create new node and set forward-link (back-link remains nullptr)
            if (m_first_ptr) m_first_ptr->m_prev_ptr = new_node_ptr; // update back-link
            m_first_ptr = new_node_ptr; // update head pointer
        }

        // precondition: node_address is in the list
        constexpr
        void
        remove(std::byte * node_address)
        {
            auto node_ptr = reinterpret_cast<node *>(node_address);
            
            // update previous node
            if (node_ptr->m_prev_ptr == nullptr) // removing first block
            {
                m_first_ptr = node_ptr->m_next_ptr;
            }
            else
            {
                node_ptr->m_prev_ptr->m_next_ptr = node_ptr->m_next_ptr;
            }

            // update next node
            if (node_ptr->m_next_ptr != nullptr)
            {
                node_ptr->m_next_ptr->m_prev_ptr = node_ptr->m_prev_ptr;
            }

            // call node destructor
            node_ptr->~node();

            // NOTE: we do not deallocate.
            // memory is not managed by this class.
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
