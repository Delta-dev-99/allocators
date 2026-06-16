#pragma once
#include <cstddef>
#include <cstdint>


namespace dd99::memory
{
    
    inline constexpr
    std::byte *
    align_up(std::byte * ptr, std::size_t alignment)
    {
        // TODO: assert alignment is power-of-2
        auto address = reinterpret_cast<std::uintptr_t>(ptr);
        auto aligned = (address + alignment - 1) & ~(alignment - 1);
        return reinterpret_cast<std::byte *>(aligned);
    }

    inline constexpr
    bool
    is_aligned(std::byte * ptr, std::size_t alignment)
    {
        return (reinterpret_cast<std::uintptr_t>(ptr) & (alignment - 1)) == 0;
        // return (reinterpret_cast<std::uintptr_t>(ptr) % alignment) == 0;
    }

}
