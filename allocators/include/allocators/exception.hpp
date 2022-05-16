#pragma once

#include <exception>

namespace dd99::memory
{
    // Base for all exceptions thrown in the library
    struct allocator_exception : std::exception
    {
        virtual ~allocator_exception() noexcept = default;
        allocator_exception() noexcept {};

        // TODO: Consider putting literals on shared lib
        virtual const char *
        what() const noexcept
        { return "dd99::memory::allocator_exception"; }
    };

    // Base for all allocation failure exceptions
    struct failed_allocation_exception : allocator_exception
    {
        virtual ~failed_allocation_exception() noexcept = default;
        failed_allocation_exception() noexcept {};

        // TODO: Consider putting literals on shared lib
        virtual const char *
        what() const noexcept
        { return "dd99::memory::failed_allocation_exception"; }
    };

    // Base for all deallocation failure exceptions
    struct failed_deallocation_exception : allocator_exception
    {
        virtual ~failed_deallocation_exception() noexcept = default;
        failed_deallocation_exception() noexcept {};

        // TODO: Consider putting literals on shared lib
        virtual const char *
        what() const noexcept
        { return "dd99::memory::failed_deallocation_exception"; }
    };

    // Deallocation failed because memory is not owned by the allocator
    struct memory_not_owned_exception : failed_deallocation_exception
    {
        virtual ~memory_not_owned_exception() noexcept = default;
        memory_not_owned_exception() noexcept {};

        // TODO: Consider putting literals on shared lib
        virtual const char *
        what() const noexcept
        {
            return "dd99::memory::memory_not_owned_exception";
        }
    };

    // thrown when blocks are too small to hold allocation data
    // or too big for the available memory
    struct invalid_block_size : allocator_exception
    {
        virtual ~invalid_block_size() noexcept = default;
        invalid_block_size() noexcept {};

        // TODO: Consider putting literals on shared lib
        virtual const char *
        what() const noexcept
        {
            return "dd99::memory::invalid_block_size";
        }
    };

    // memory has been corrupted
    struct memory_corrupted : allocator_exception
    {
        virtual ~memory_corrupted() noexcept = default;
        memory_corrupted() noexcept {};

        // TODO: Consider putting literals on shared lib
        virtual const char *
        what() const noexcept
        {
            return "dd99::memory::memory_corrupted";
        }
    };

}
