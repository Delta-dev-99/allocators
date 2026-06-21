#pragma once

#include <allocators/structures/blocks/raii_block.hpp>

namespace dd99::memory::block_allocator::utility
{

    // NOTE: This class does NOT inherit from Allocator base class.
    // NOTE: This class is not suitable for allocator polymorphism.
    // returns Unique_Block on allocation.
    // Blocks are deallocated automatically on destruction.
    // NOTE: The sub allocator cannot be degenerate.
    // TODO: Add assignment operator definitions
    template <class Sub_Alloc_T>
    class raii_block_allocator
    {
    public:

        struct deallocator_type
        {
            // used ptr to allow assignment
            // used 2nd level ptr to allow moving
            raii_block_allocator ** allocator = nullptr;
            
            constexpr void
            operator()(const memory::block & memory) const
            {
                if (allocator && *allocator)
                    (*allocator)->deallocate(memory);
            }
        };

        using block_type = dd99::memory::raii_block<deallocator_type>;

    public:
        ~raii_block_allocator()
        {
            cleanup();
        }

        raii_block_allocator(Sub_Alloc_T && sub_allocator)
            : m_sub_alloc(std::move(sub_allocator))
            , m_self_ptr_block(m_sub_alloc.allocate(get_memory_overhead()))
        {
            auto ptr = new (m_self_ptr_block.base) raii_block_allocator *;
            *ptr = this;
        }

        raii_block_allocator(raii_block_allocator && other)
            : m_sub_alloc(std::move(other.m_sub_alloc))
            , m_self_ptr_block(std::move(other.m_self_ptr_block))
        {
            *reinterpret_cast<raii_block_allocator **>(m_self_ptr_block.base) = this;
            other.m_self_ptr_block = {};
        }


        raii_block_allocator & operator=(raii_block_allocator && other)
        {
            if (this != & other)
            {
                cleanup();

                m_sub_alloc = std::move(other.m_sub_alloc);
                m_self_ptr_block = std::move(other.m_self_ptr_block);

                *reinterpret_cast<raii_block_allocator **>(m_self_ptr_block.base) = this;
                other.m_self_ptr_block = {};
            }
            return *this;
        }

    public:
        static constexpr
        std::size_t get_memory_overhead()
        { return sizeof(raii_block_allocator *); }

    public:
        [[nodiscard]]
        block_type
        allocate(std::size_t requested_size)
        {
            auto mem = m_sub_alloc.allocate(requested_size);
            auto deallocator = deallocator_type{reinterpret_cast<raii_block_allocator **>(m_self_ptr_block.base)};
            return block_type{std::move(mem), std::move(deallocator)};
        }

        void deallocate(const memory::block &memory)
        {
            if (!owns(memory)) return;
            return m_sub_alloc.deallocate(memory);
        }

        void deallocate_all() { m_sub_alloc.deallocate_all(); }

        bool owns(const std::byte * memory) const { return m_sub_alloc.owns(memory); }

        bool owns(const memory::block &memory) const { return m_sub_alloc.owns(memory); }

    protected:
        void cleanup()
        {
            auto self_ptr_ptr = reinterpret_cast<raii_block_allocator **>(m_self_ptr_block.base);
            if (self_ptr_ptr)
                *self_ptr_ptr = nullptr;
            m_sub_alloc.deallocate(m_self_ptr_block);
        }
    
    private:
        Sub_Alloc_T m_sub_alloc;
        memory::block m_self_ptr_block;
    };

}