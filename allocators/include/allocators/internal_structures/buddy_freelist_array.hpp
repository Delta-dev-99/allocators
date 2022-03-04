#pragma once

#include <allocators/internal_structures/free_list.hpp>

namespace dd99::memory::structure::detail
{
    // TODO: Make constexpr
    // Purpose: Initialize the freelists with apropiate sizes
    template <unsigned Levels, std::size_t... Sizes>
    class Buddy_Freelist_Array_Base_Impl
    {
    protected:
        memory::structure::Freelist_Double_Link m_freelists[Levels];

    public:
        Buddy_Freelist_Array_Base_Impl()
            : m_freelists{Sizes...}
        { }
    };



    template <std::size_t BLOCK_SIZE,
                unsigned Levels,
                std::size_t Current_BLOCK_SIZE = BLOCK_SIZE,
                std::size_t Remaining_Levels = Levels,
                std::size_t... Sizes>
    class Buddy_Freelist_Array_Base : public Buddy_Freelist_Array_Base<BLOCK_SIZE, Levels, Current_BLOCK_SIZE * 2, Remaining_Levels - 1, Sizes..., Current_BLOCK_SIZE>
    { };

    template <std::size_t BLOCK_SIZE,
                unsigned Levels,
                std::size_t Current_BLOCK_SIZE,
                std::size_t... Sizes>
    class Buddy_Freelist_Array_Base<BLOCK_SIZE, Levels, Current_BLOCK_SIZE, 0, Sizes...> : public Buddy_Freelist_Array_Base_Impl<Levels, Sizes...>
    { };
}
