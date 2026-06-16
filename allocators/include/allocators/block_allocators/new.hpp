#pragma once

#include <allocators/library_configuration/cpp_config.hpp>
#include <allocators/block_allocators/block_allocator.hpp>
#include <allocators/structures/new_result.hpp>
#include <allocators/alignment.hpp>
#include <type_traits>

namespace dd99::memory
{
    template <class T, Block_Allocator Allocator>
    requires std::is_object_v<T> && (!std::is_unbounded_array_v<T>)
    auto allocator_new(Allocator & allocator)
    {
        constexpr auto size = sizeof(T);
        constexpr auto alignment = alignof(T);

        new_result<T> result{allocator.allocate(size, alignment)};

        DD99_ALLOCATORS_ASSERT_DEBUG("allocation result has wrong alignment", is_aligned(result.pointer, alignment));
        DD99_ALLOCATORS_ASSERT_DEBUG("allocation result has wrong size", (result.size >= size) || (result.size == 0));
        
        return std::move(result);
    }

    // allocate array
    template <class T, Block_Allocator Allocator>
    requires std::is_unbounded_array_v<T>
    auto allocator_new(Allocator & allocator, std::size_t count)
    {
        constexpr auto size = sizeof(std::remove_extent_t<T>);
        constexpr auto alignment = alignof(std::remove_extent_t<T>);

        new_result<T> result{allocator.allocate(size, alignment)};

        DD99_ALLOCATORS_ASSERT_DEBUG("allocation result has wrong alignment", is_aligned(result.pointer, alignment));
        DD99_ALLOCATORS_ASSERT_DEBUG("allocation result has wrong size", (result.size >= size) || (result.size == 0));

        return std::move(result);
    }


    template <class T, class Allocator>
    void allocator_delete(Allocator & allocator, dd99::memory::new_result<T> mem)
    {
        allocator.deallocate(mem);
    }
}
