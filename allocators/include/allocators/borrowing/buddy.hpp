#pragma once

#include <allocators/internal/bases/buddy.hpp>
#include <allocators/basic/stack.hpp>
#include <allocators/utility/unique_block.hpp>
#include <allocators/composite/throwing.hpp>


// borrowing allocators borrow memory from other allocators to place inner state
namespace dd99::memory::block_allocator::borrowing
{

    template <std::size_t BLOCK_SIZE,
              unsigned LEVELS,
              class Sub_Alloc_T = dd99::memory::block_allocator::Stack,
              class Bitmap_Block_Type = std::byte>
    class Buddy
        : public dd99::memory::block_allocator::internal::base::Buddy_Base<BLOCK_SIZE, LEVELS, Bitmap_Block_Type>
    {
    protected: // internal type definitions
        using Buddy_Base = dd99::memory::block_allocator::internal::base::Buddy_Base<BLOCK_SIZE, LEVELS, Bitmap_Block_Type>;
        using typename Buddy_Base::BMP;

        using Aux_Allocator =
            dd99::memory::block_allocator::utility::Unique_Block_Allocator<
                dd99::memory::block_allocator::composite::Throwing<Sub_Alloc_T>>;
        using Aux_Block = typename Aux_Allocator::Block_Type;

    public: // constant definitions
        using Buddy_Base::Levels;
        using Buddy_Base::Block_Size;
        using Buddy_Base::Max_Block_Size;

    public: // static functions
        static constexpr
        std::size_t calculate_block_count(std::size_t memory_size)
        {
            return memory_size / Block_Size;
        }

    private:
        static constexpr
        std::size_t calculate_aux_allocation(std::size_t block_count)
        {
            const auto bit_count = Buddy_Base::calculate_buddy_bit_count(block_count);
            const auto bmp_blocks = BMP::calculate_block_count(bit_count);
            const auto bmp_bytes = bmp_blocks * BMP::Block_Size;
            return bmp_bytes;
        }

        // static constexpr
        // std::size_t calculate_aux_allocation(std::size_t memory_size)
        // {
        //     const auto block_count = calculate_block_count(memory_size);
        //     return calculate_aux_allocation_from_block_count(block_count);
        // }

    public:
        // only provided for client use
        static constexpr
        std::size_t calculate_aux_mem_size(std::size_t main_mem_size)
        {
            const auto block_count = calculate_block_count(main_mem_size);
            const auto aux_allocation_size = calculate_aux_allocation(block_count);
            return aux_allocation_size + Aux_Allocator::get_memory_overhead();
        }

    private: // internal constructors
        Buddy(memory::Block memory,
              Aux_Allocator && aux_allocator,
              std::size_t block_count,
              Aux_Block && aux_block)
            : Buddy_Base(memory, block_count, aux_block.base)
            , m_aux_allocator(std::move(aux_allocator))
            , m_aux_memory(std::move(aux_block))
        {
            deallocate_all();
        }

        Buddy(memory::Block memory,
              Aux_Allocator && aux_allocator,
              std::size_t block_count)
            : Buddy(memory, std::move(aux_allocator),
                    block_count,
                    aux_allocator.allocate(calculate_aux_allocation(block_count)))
        { }

    public: // constructor
        Buddy(memory::Block memory, Sub_Alloc_T && sub_allocator)
            : Buddy(memory, Aux_Allocator{std::move(sub_allocator)}, calculate_block_count(memory.size))
        { }

    public: // allocator interface implementation
        using Buddy_Base::allocate;
        using Buddy_Base::deallocate;
        using Buddy_Base::deallocate_all;
        using Buddy_Base::owns;

    protected:
        constexpr
        std::byte *
        get_blocks_base() const override
        {
            return Buddy_Base::m_memory.base;
        }

    private: // members
        Aux_Allocator m_aux_allocator;
        Aux_Block m_aux_memory;
    };
    
}
