#pragma once

#include <allocators/library_configuration/cpp_config.hpp>
#include <cstdint>
#include <utility>
#include <cstddef>


namespace dd99_allocators_namespace
{

    // A block of memory.
    // Described by base address and size in bytes.
    struct block
    {
        constexpr std::byte * get_base() const noexcept { return base; }
        constexpr std::size_t get_size() const noexcept { return size; }
        constexpr std::byte * get_end() const noexcept { return get_base() + get_size(); }
        
        constexpr
        bool
        contains(const block & other) const noexcept
        {
            // const auto base_offset = other.base - base;
            // return (base_offset >= 0) && (base + base_offset + other.size <= get_end());

            return (other.get_base() >= get_base()) && (other.get_end() <= get_end());
        }

        constexpr
        bool
        contains(const std::byte * ptr) const noexcept
        {
            return (get_base() <= ptr) && (get_end() > ptr);
        }

        constexpr
        bool
        empty() const noexcept
        {
            return (get_size() == 0);
        }

        // check if block is empty
        constexpr
        operator bool() const noexcept
        {
            return !empty();
        }


        std::byte * base = nullptr;
        std::size_t size = 0;
    };


    // TODO: add implicit construction from arrays with appropriate types.

    

}
