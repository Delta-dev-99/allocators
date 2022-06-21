#pragma once

#include <allocators/pointer/allocator.hpp>

namespace dd99::memory
{
    template <class T, Pointer_Allocator Allocator = dd99::memory::pointer_allocator::Allocator>
    requires std::is_object_v<T> && (!std::is_unbounded_array_v<T>)
    T * allocator_new(Allocator & allocator)
    {
        return allocator.allocate(sizeof(T));
    }

    // allocate array
    template <class T, Pointer_Allocator Allocator = dd99::memory::block_allocator::Allocator>
    requires std::is_unbounded_array_v<T>
    T * allocator_new(Allocator & allocator, std::size_t count)
    {
        // TODO: Verify alignment
        return allocator.allocate(count * sizeof(std::remove_extent_t<T>));
    }
}
