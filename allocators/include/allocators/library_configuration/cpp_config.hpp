#pragma once
#include <string_view>

// This header provides user-facing compile-time settings for the whole library.



// Assertions:
// ###########

#if not defined(DD99_ALLOCATOR_ASSERTIONS) and (not defined(NDEBUG) or not NDEBUG)
#define DD99_ALLOCATOR_ASSERTIONS_ENABLED true
#endif



#if defined(DD99_ALLOCATOR_ASSERTIONS_ENABLED) and DD99_ALLOCATOR_ASSERTIONS_ENABLED

#include <format>
#include <iostream>

namespace dd99::memory
{
    inline constexpr void dd99_allocator_assert_failed(std::string_view expr_sv, std::string_view file, std::size_t line)
    {
        std::cout << std::format("allocator assertion failed! source: {}:{} expression: {}\n", file, line, expr_sv);
    }
}


#if defined(DD99_ALLOCATOR_ASSERTIONS_ENABLED) and DD99_ALLOCATOR_ASSERTIONS_ENABLED and not defined(DD99_ALLOCATOR_ASSERT)
#define DD99_ALLOCATOR_ASSERT(...) if (!(__VA_ARGS__)) dd99::memory::dd99_allocator_assert_failed(#__VA_ARGS__, __FILE__, __LINE__);
#endif



#else // DD99_ALLOCATOR_ASSERTIONS_ENABLED



#define DD99_ALLOCATOR_ASSERT(...) (static_cast<void>(0))



#endif // DD99_ALLOCATOR_ASSERTIONS_ENABLED
