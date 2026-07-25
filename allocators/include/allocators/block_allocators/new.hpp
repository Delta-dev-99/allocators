#pragma once

#include <allocators/library_configuration/cpp_config.hpp>
#include <allocators/block_allocators/block_allocator.hpp>
#include <allocators/structures/blocks/raii_block.hpp>
#include <allocators/structures/new_result.hpp>
#include <allocators/alignment.hpp>
#include <type_traits>

namespace dd99_allocators_namespace
{

    namespace detail
    {
        template <class T>
        constexpr void destroy_at(T * ptr)
        {
            if constexpr (std::is_array_v<T>)
            {
                for (auto i = std::extent_v<T>; i-- > 0;)
                {
                    destroy_at(ptr + i);
                }
            }
            else
            {
                ptr->~T();
            }
        }
    }

    // T (non-array)
    template <class T, Block_Allocator Allocator, class ... Args>
    requires std::is_object_v<T> && (!std::is_array_v<T>)
    new_result<T>
    allocator_new(Allocator & allocator, Args && ... args)
    {
        auto blk = allocate_raii_block(allocator, sizeof(T), alignof(T));
        ::new (blk.get_base()) T{std::forward<Args>(args)...};
        return new_result<T>{blk.release()};
    }

    // T[N]
    template <class T, Block_Allocator Allocator>
    requires std::is_object_v<T> && (std::is_bounded_array_v<T>)
    new_result<T>
    allocator_new(Allocator & allocator)
    {
        // TODO: assert(extent<T> != 0)
        // NOTE: raii_block provides exception safety in case any constructor throws.
        // NOTE: if a constructor throws, the new-expression handles the destruction of already constructed objects.

        auto blk = allocate_raii_block(allocator, sizeof(T), alignof(T));
        ::new (blk.get_base()) T{};
        return new_result<T>{blk.release()};
    }

    // T[]
    template <class T, Block_Allocator Allocator>
    requires std::is_object_v<T> && (std::is_unbounded_array_v<T>)
    new_result<T>
    allocator_new(Allocator & allocator, std::size_t count)
    {
        // assert(count > 0)

        using type = std::remove_extent_t<T>;
        auto blk = allocate_raii_block(allocator, sizeof(type) * count, alignof(type));
        
        std::size_t i;
        try
        {
            for (i = 0; i < count; ++i)
            {
                ::new (blk.get_base() + sizeof(type) * i) type{};
            }
        }
        catch(...)
        {
            for (; i-- > 0;)
            {
                detail::destroy_at(reinterpret_cast<type *>(blk.get_base() + sizeof(type) * (i)));
            }
            throw;
        }

        return new_result<T>{blk.release(), count};
    }


    template <class T, Block_Allocator Allocator>
    void
    allocator_delete(Allocator & allocator, new_result<T> & res)
    {
        // assert (res.get() != nullptr)

        // destroy objects
        if constexpr (std::is_array_v<T>)
        {
            for (auto i = res.get_count(); i-- > 0;)
            {
                detail::destroy_at(res.get() + i);
            }
        }
        else
        {
            detail::destroy_at(res.get());
        }

        // deallocate
        allocator.deallocate(res.get_block());
    }


    // template <class T, Block_Allocator Allocator>
    // requires std::is_object_v<T> && (!std::is_unbounded_array_v<T>)
    // auto allocator_new(Allocator & allocator)
    // {
    //     constexpr auto size = sizeof(T);
    //     constexpr auto alignment = alignof(T);

    //     new_result<T> result{allocator.allocate(size, alignment)};

    //     DD99_ALLOCATORS_ASSERT_DEBUG("allocation result has wrong alignment", is_aligned(result.get(), alignment));
    //     DD99_ALLOCATORS_ASSERT_DEBUG("allocation result has wrong size", (result.get_size() >= size) || (result.get_size() == 0));
        
    //     return std::move(result);
    // }

    // // allocate array
    // template <class T, Block_Allocator Allocator>
    // requires std::is_unbounded_array_v<T>
    // auto allocator_new(Allocator & allocator, std::size_t count)
    // {
    //     // TODO: *** what to do for types that require padding in an array? is the padding included in `sizeof`?

    //     constexpr auto element_size = sizeof(std::remove_extent_t<T>);
    //     constexpr auto alignment = alignof(std::remove_extent_t<T>);
    //     const auto size = element_size * count;

    //     new_result<T> result{allocator.allocate(size, alignment), count};

    //     DD99_ALLOCATORS_ASSERT_DEBUG("allocation result has wrong alignment", is_aligned(result.get(), alignment));
    //     DD99_ALLOCATORS_ASSERT_DEBUG("allocation result has wrong size", (result.get_size() >= size) || (result.get_size() == 0));

    //     return std::move(result);
    // }


    // template <class T, class Allocator>
    // void allocator_delete(Allocator & allocator, dd99_allocators_namespace::new_result<T> mem)
    // {
    //     // TODO: destroy objects
    //     allocator.deallocate(mem.get_block());
    // }
}
