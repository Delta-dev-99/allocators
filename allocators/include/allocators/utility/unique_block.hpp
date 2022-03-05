#pragma once

#include <allocators/internal_structures/memory_block.hpp>

namespace dd99::memory::block_allocator::composite
{

    // NOTE: This class does NOT inherit from Allocator base class.
    // NOTE: This class is not suitable for allocator polymorphism.
    // returns Unique_Block on allocation.
    // Blocks are deallocated automatically on destruction.
    template <class Sub_Alloc_T>
    class Unique_Block_Allocator
    {
    public:
        struct Deallocator
        {
            // used ptr to allow assignment
            Unique_Block_Allocator * allocator = nullptr;
            
            constexpr void
            operator()(const memory::Block & memory) const
            {
                if (allocator)
                    allocator->deallocate(memory);
            }
        };

        using Block_Type = dd99::memory::Unique_Block<Deallocator>;

    public:
        Unique_Block_Allocator(Sub_Alloc_T && sub_allocator)
            : m_sub_alloc(std::move(sub_allocator))
        { }

    public:
        [[nodiscard]]
        Block_Type
        allocate(std::size_t requested_size)
        {
            return Block_Type{m_sub_alloc.allocate(requested_size), Deallocator{this}};
        }

        void deallocate(const memory::Block &memory)
        {
            if (!owns(memory)) return;
            return m_sub_alloc.deallocate(memory);
        }

        void deallocate_all() { m_sub_alloc.deallocate_all(); }

        bool owns(std::byte *memory) const { return m_sub_alloc.contains(memory); }

        bool owns(const memory::Block &memory) const { return m_sub_alloc.owns(memory); }
    
    private:
        Sub_Alloc_T m_sub_alloc;
    };

}