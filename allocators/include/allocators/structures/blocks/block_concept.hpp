#pragma once

#include <allocators/structures/blocks/memory_block.hpp>
#include <concepts>



namespace dd99_allocators_namespace
{

    template <class T>
    concept Block_Concept = requires(T & blk, const block & other_blk, const std::byte * bptr)
    {
        { blk.get_base() } -> std::convertible_to<std::byte *>;
        { blk.get_size() } -> std::convertible_to<std::size_t>;
        { blk.get_end() } -> std::convertible_to<std::byte *>;
        { blk.contains(other_blk) } -> std::convertible_to<bool>;
        { blk.contains(bptr) } -> std::convertible_to<bool>;
        { blk.empty() } -> std::convertible_to<bool>;
        static_cast<bool>(blk); // check empty block
    };

    template <class T>
    concept Movable_Block = Block_Concept<T> && std::movable<T>;

}
