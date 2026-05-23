#pragma once

#include <allocators/pointer_allocators/allocator.hpp>

namespace dd99::memory
{
    template <class T, Pointer_Allocator Allocator>
    requires std::is_object_v<T> && (!std::is_unbounded_array_v<T>)
    T * allocator_new(Allocator & allocator)
    {
        return allocator.allocate(sizeof(T), alignof(T));
    }

    // allocate array
    template <class T, Pointer_Allocator Allocator>
    requires std::is_unbounded_array_v<T>
    T * allocator_new(Allocator & allocator, std::size_t count)
    {
        // TODO: Verify alignment and padding
        return allocator.allocate(count * sizeof(std::remove_extent_t<T>), alignof(T));
    }
}
