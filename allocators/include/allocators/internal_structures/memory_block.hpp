#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <cstddef>


namespace dd99::memory
{
    // A block of memory.
    // Described by base address and size in bytes.
    struct Block
    {
        std::byte * base = nullptr;
        std::size_t size = 0;

        constexpr
        std::byte *
        get_end() const { return base + size; }
        
        constexpr
        bool
        contains(const Block & other) const
        {
            const auto base_offset = other.base - base;
            return (base_offset >= 0) && (base + base_offset + other.size <= get_end());
        }

        constexpr
        bool contains(void * ptr) const
        {
            return (base <= ptr) && (get_end() >= ptr);
        }

        // check if block is empty
        constexpr
        operator bool() const
        {
            return size;
        }
    };

    // Contains the memory it describes.
    // Facilitates acquiring memory from the Stack.
    // NOTE: Be careful with this one.
    // NOTE: The
    template <std::size_t Size>
    struct Self_Contained_Block : Block
    {
        static_assert(Size > 0);

        constexpr
        Self_Contained_Block()
            : Block{.base = m_data, .size = Size}
        { }

        Self_Contained_Block(const Self_Contained_Block& other) = delete;
        Self_Contained_Block(Self_Contained_Block&& other) = delete;

        std::byte m_data[Size];
    };

    // Facilitates acquiring memory from the Heap.
    // Releases the memory on destruction (RAII).
    // NOTE: This structure defeats the purpose of the library.
    // NOTE: This auto acquired memory block uses new to get memory.
    template <std::size_t Size>
    struct Heap_Block : Block
    {
        ~Heap_Block()
        {
            delete[] base;
            // free(base);
        }

        Heap_Block()
        {
            base = new std::byte[Size];
            // base = malloc(Size);
            if (!base) throw std::bad_alloc{};
            size = Size;
        }
    };
}
