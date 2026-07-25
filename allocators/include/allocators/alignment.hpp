#pragma once
#include <allocators/library_configuration/cpp_config.hpp>
#include <cstddef>
#include <cstdint>
// #include <concepts>
#include <bit>


namespace dd99_allocators_namespace
{
    
    inline constexpr
    std::byte *
    align_up(std::byte * ptr, std::size_t alignment)
    {
        DD99_ALLOCATORS_ASSERT_HARDENED("alignment must be a power of 2", std::has_single_bit(alignment));

        auto address = reinterpret_cast<std::uintptr_t>(ptr);
        auto aligned = (address + alignment - 1) & ~(alignment - 1);
        return reinterpret_cast<std::byte *>(aligned);
    }

    template <class T>
    inline constexpr
    bool
    is_aligned(T * ptr, std::size_t alignment)
    {
        DD99_ALLOCATORS_ASSERT_HARDENED("alignment must be a power of 2", std::has_single_bit(alignment));
        
        return (reinterpret_cast<std::uintptr_t>(ptr) & (alignment - 1)) == 0;
        // return (reinterpret_cast<std::uintptr_t>(ptr) % alignment) == 0;
    }

}
