#pragma once

#include <allocators/block_allocators/basic/buddy/buddy_state.hpp>
#include <allocators/structures/blocks/memory_block.hpp>
#include <algorithm>
// #include <cassert>


namespace dd99::memory::block_allocator
{

    template <buddy_namespace::State_Concept State_Type>
    class buddy
    {
    public: // constant definitions
        using state_type = State_Type;
        using layout_type = state_type::layout_type;
        using level_type = state_type::level_type;
        using index_type = state_type::index_type;

        static constexpr auto block_size = state_type::block_size;
        static constexpr auto levels = state_type::levels;
        static constexpr auto last_level = state_type::last_level;

    public: // constructors
        buddy(state_type state)
            : m_state{std::move(state)}
        { }

    private:
        [[nodiscard]]
        constexpr
        std::byte * allocate_impl(level_type level)
        {
            // recursion termination condition
            if (level > last_level) return nullptr;

            // try freelist
            if (auto block_base = m_state.pop(level)) return block_base;

            // get larger block
            auto block_base = allocate_impl(level+1); // recursion
            if (!block_base) return nullptr;
            
            // split larger block
            auto half_size = layout_type::get_level_block_size(level);
            auto buddy_base = block_base + half_size;
            m_state.push(level, buddy_base);
            return block_base;
        }

    public: // allocator interface implementation
        [[nodiscard]]
        constexpr
        memory::block
        allocate_level(level_type requested_level)
        {
            return block{.base = allocate_impl(requested_level), .size = layout_type::get_block_size(requested_level)};
        }

        [[nodiscard]]
        constexpr
        memory::block
        allocate(std::size_t requested_size, std::size_t requested_alignment)
        {
            // TODO: consider, for alignment larger than requested size we can allocate from a higher level and split the block.
            // TODO: assert requested_alignment is a power of 2
            if (requested_size == 0) return {};
            const auto size_level = layout_type::calculate_block_level(requested_size);
            const auto alignment_level = std::max(size_level, layout_type::get_alignment_level(requested_alignment));
            
            // now, get a block from the alignment level and split it down to the size level
            auto block_level = alignment_level;
            auto block_base = allocate_impl(block_level);
            if (!block_base) return {}; // allocation failed. we need to check to avoid pushing nullptr into the freelist.
            while (block_level > size_level)
            {
                --block_level;
                auto half_size = layout_type::get_level_block_size(block_level);
                auto buddy_base = block_base + half_size;
                m_state.push(block_level, buddy_base);
            }

            return block{.base = block_base, .size = layout_type::get_level_block_size(size_level)};
        }

        [[nodiscard]]
        constexpr
        memory::block
        allocate(std::size_t requested_size)
        {
            if (requested_size == 0) return {};
            const auto requested_level = layout_type::calculate_block_level(requested_size);
            return allocate_level(requested_level);
        }


        constexpr
        void
        deallocate(block blk)
        {
            if (!owns(blk)) return;

            auto level = layout_type::get_block_level(blk.size);
            auto block_base = blk.base;
            while((block_base = m_state.merge_or_push(level, block_base)))
            {
                ++level;
            }
        }

        constexpr
        void
        deallocate_all()
        {
            m_state.reset();
        }

        constexpr
        bool
        owns(const std::byte * blk_base) const
        {
            return m_state.m_layout.m_memory.contains(blk_base);
        }

        constexpr
        bool
        owns(block blk) const
        {
            return m_state.m_layout.m_memory.contains(blk);
        }


    private:
        state_type m_state;
    };

}
