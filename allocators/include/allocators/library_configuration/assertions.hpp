#pragma once

// FILE: "allocators/library_configuration/assertions.hpp"

#include <string_view>


#define DD99_ALLOCATORS_ASSERT_LEVEL_NONE       0
#define DD99_ALLOCATORS_ASSERT_LEVEL_CRITICAL   1
#define DD99_ALLOCATORS_ASSERT_LEVEL_HARDENED   2
#define DD99_ALLOCATORS_ASSERT_LEVEL_DEBUG      3

// default definition of DD99_ALLOCATORS_ASSERT_LEVEL when no definition is provided by the user
#ifndef DD99_ALLOCATORS_ASSERT_LEVEL
#  ifdef NDEBUG
#    define DD99_ALLOCATORS_ASSERT_LEVEL    DD99_ALLOCATORS_ASSERT_LEVEL_CRITICAL
#  else
#    define DD99_ALLOCATORS_ASSERT_LEVEL    DD99_ALLOCATORS_ASSERT_LEVEL_DEBUG
#  endif
#endif


namespace dd99::memory
{

    struct assertion_info {
        std::string_view expression;
        std::string_view file;
        unsigned int line;
        // std::string_view function;
        unsigned int level; // assertion level
        std::string_view message; // may be empty
    };

    // declaration-only in the library; definition provided by the user.
    // default definition may be provided as a weak symbol.
    // must be [[noreturn]]; the library assumes violations never return.
    [[noreturn]] void allocators_assertion_failed(const assertion_info &) noexcept;

}


#define DD99_ALLOCATORS_DO_ASSERT(LEVEL, MESSAGE, ...) do { if (!(__VA_ARGS__)) dd99::memory::allocators_assertion_failed({.expression = (#__VA_ARGS__), .file = __FILE__, .line = __LINE__, .level = LEVEL, .message = (MESSAGE)}) } while (false)


#if DD99_ALLOCATORS_ASSERT_LEVEL < DD99_ALLOCATORS_ASSERT_LEVEL_CRITICAL
#  define DD99_ALLOCATORS_ASSERT_CRITICAL(MESSAGE, ...) (static_cast<void>(0))
#else
#  define DD99_ALLOCATORS_ASSERT_CRITICAL(MESSAGE, ...) DD99_ALLOCATORS_DO_ASSERT(DD99_ALLOCATORS_ASSERT_LEVEL_CRITICAL, MESSAGE, __VA_ARGS__)
#endif

#if DD99_ALLOCATORS_ASSERT_LEVEL < DD99_ALLOCATORS_ASSERT_LEVEL_HARDENED
#  define DD99_ALLOCATORS_ASSERT_HARDENED(MESSAGE, ...) (static_cast<void>(0))
#else
#  define DD99_ALLOCATORS_ASSERT_HARDENED(MESSAGE, ...) DD99_ALLOCATORS_DO_ASSERT(DD99_ALLOCATORS_ASSERT_LEVEL_HARDENED, MESSAGE, __VA_ARGS__)
#endif

#if DD99_ALLOCATORS_ASSERT_LEVEL < DD99_ALLOCATORS_ASSERT_LEVEL_DEBUG
#  define DD99_ALLOCATORS_ASSERT_DEBUG(MESSAGE, ...) (static_cast<void>(0))
#else
#  define DD99_ALLOCATORS_ASSERT_DEBUG(MESSAGE, ...) DD99_ALLOCATORS_DO_ASSERT(DD99_ALLOCATORS_ASSERT_LEVEL_DEBUG, MESSAGE, __VA_ARGS__)
#endif


// *** NOTES on configuring assertions
// A hardened release build should use -DNDEBUG -DDD99_ALLOCATORS_ASSERT_LEVEL=2. A kernel that wants everything off may use level 0. The defaults do the right thing for ordinary development without any configuration.
// The three tiers mean different things:
// `Critical` fires in all builds by default. These are conditions that indicate the allocator was constructed incorrectly or that its memory has been externally corrupted — situations where continuing is more dangerous than halting. State memory too small for the requested layout is one example. These should be few, cheap to check, and inarguably fatal.
// `Hardened` fires for API contract violations — preconditions that are the caller's responsibility. Misaligned managed memory, requested_alignment that is not a power of two, deallocate called with a block the allocator doesn't own. These are almost always bugs in calling code. They are off in release by default because they add overhead and production code should not be triggering them, but a security-conscious operator can enable them.
// `Debug` fires for internal invariant checks — assertions about the allocator's own state that would only be false if there is a bug in the library itself. After every structural modification in tests, a full consistency walk. These are too expensive and too "internal" for any production build, even hardened ones.
