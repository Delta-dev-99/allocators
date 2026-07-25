#pragma once

#include <allocators/structures/blocks/memory_block.hpp>

namespace dd99_allocators_namespace
{
    // The result of an allocator-new call
    template <class T>
    struct new_result
    {
        T * m_ptr = nullptr; // base of the allocated block
        static constexpr std::size_t m_count = 1; // number of allocated objects in an array
        std::size_t m_size = 0; // size of the allocated block

        constexpr new_result() noexcept = default;
        constexpr explicit new_result(block blk) noexcept
            : m_ptr{reinterpret_cast<T *>(blk.base)}
            , m_size{blk.size}
        { }

        constexpr
        block
        get_block() noexcept
        { return {reinterpret_cast<std::byte *>(m_ptr), m_size}; }

        constexpr
        std::size_t
        get_size() const noexcept
        { return m_size; }

        constexpr
        std::size_t
        get_count() const noexcept
        { return m_count; }

        constexpr
        T *
        get() const noexcept
        { return m_ptr; }

        // operator *
        constexpr
        T &
        operator*() const noexcept
        { return *m_ptr; }

        // operator ->
        constexpr
        T *
        operator->() const noexcept
        { return m_ptr; }
    };

    // specialization for arrays of known bound
    template <class T, std::size_t N>
    struct new_result<T[N]>
    {
        T * m_ptr = nullptr; // base of the allocated block
        static constexpr std::size_t m_count = N; // number of allocated objects in an array
        std::size_t m_size = 0; // size of the allocated block

        constexpr new_result() noexcept = default;
        constexpr explicit new_result(block blk) noexcept
            : m_ptr{reinterpret_cast<T *>(blk.base)}
            , m_size{blk.size}
        { }

        constexpr
        block
        get_block() noexcept
        { return {reinterpret_cast<std::byte *>(m_ptr), m_size}; }

        constexpr
        std::size_t
        get_size() const noexcept
        { return m_size; }

        constexpr
        std::size_t
        get_count() const noexcept
        { return m_count; }

        constexpr
        T *
        get() const noexcept
        { return m_ptr; }

        // operator *
        constexpr
        T &
        operator*() const noexcept
        { return *m_ptr; }

        // operator ->
        constexpr
        T *
        operator->() const noexcept
        { return m_ptr; }

        // operator []
        constexpr
        T &
        operator[](std::size_t index) const noexcept
        { return m_ptr[index]; }
    };

    // specialization for arrays of unknown bound
    template <class T>
    struct new_result<T[]>
    {
        T * m_ptr = nullptr; // base of the allocated block
        std::size_t m_count = 0; // number of allocated objects in an array
        std::size_t m_size = 0; // size of the allocated block

        constexpr new_result() noexcept = default;
        constexpr explicit new_result(block blk, std::size_t count) noexcept
            : m_ptr{reinterpret_cast<T *>(blk.base)}
            , m_count{count}
            , m_size{blk.size}
        { }

        constexpr
        block
        get_block() noexcept
        { return {reinterpret_cast<std::byte *>(m_ptr), m_size}; }

        constexpr
        std::size_t
        get_size() const noexcept
        { return m_size; }

        constexpr
        std::size_t
        get_count() const noexcept
        { return m_count; }

        constexpr
        T *
        get() const noexcept
        { return m_ptr; }

        // operator *
        constexpr
        T &
        operator*() const noexcept
        { return *m_ptr; }

        // operator ->
        constexpr
        T *
        operator->() const noexcept
        { return m_ptr; }

        // operator []
        constexpr
        T &
        operator[](std::size_t index) const noexcept
        { return m_ptr[index]; }
    };
}
