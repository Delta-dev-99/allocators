#pragma once

#include <allocators/allocator.hpp>
#include <allocators/structures/new_result.hpp>
#include <type_traits>

namespace dd99::memory
{
    template <class T, Block_Allocator Allocator>
    requires std::is_object_v<T> && (!std::is_unbounded_array_v<T>)
    auto allocator_new(Allocator & allocator)
    {
        // TODO: Verify alignment
        return new_result<T>{allocator.allocate(sizeof(T))};
    }

    // allocate array
    template <class T, Block_Allocator Allocator>
    requires std::is_unbounded_array_v<T>
    auto allocator_new(Allocator & allocator, std::size_t count)
    {
        // TODO: Verify alignment
        return new_result<T>{allocator.allocate(count * sizeof(std::remove_extent_t<T>))};
    }


    template <class T, class Allocator>
    void allocator_delete(Allocator & allocator, dd99::memory::new_result<T> mem)
    {
        allocator.deallocate(mem);
    }
}
