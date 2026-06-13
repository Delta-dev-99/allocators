#pragma once

#include <allocators/structures/blocks/memory_block.hpp>

namespace dd99::memory
{
    // The result of an allocator-new
    template <class T>
    struct new_result
    {
        T * pointer = nullptr;
        std::size_t size = 0;

        constexpr new_result() noexcept = default;
        constexpr explicit new_result(const dd99::memory::block block) noexcept
            : pointer(reinterpret_cast<T *>(block.base))
            , size(block.size)
        { }

        // conversion to memory block
        operator dd99::memory::block()
        { return {reinterpret_cast<std::byte *>(pointer), size}; }

        // decay to pointer
        operator T *()
        { return pointer; }

        // operator *
        constexpr T & operator*()
        { return *pointer; }

        // operator ->
        constexpr T * operator->()
        { return pointer; }
    };
}
