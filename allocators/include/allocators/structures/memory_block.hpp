#pragma once

#include <cstdint>
#include <memory>


namespace dd99::memory
{
    // A block of memory.
    // Described by base address and size in bytes.
    struct Block
    {
        void *base = nullptr;
        std::size_t size = 0;

        auto get_end() const { return reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(base) + size); }
        
        bool contains(const Block &other) const
        {
            const auto base_offset = reinterpret_cast<std::intptr_t>(other.base) - reinterpret_cast<std::intptr_t>(base);
            return (base_offset >= 0) && (other.size + base_offset <= size);
        }

        bool contains(void * ptr) const
        {
            return (base <= ptr) && (get_end() >= ptr);
        }

        // check if block is empty
        operator bool()
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

        Self_Contained_Block()
            : Block{.base = m_data, .size = Size}
        { }

        Self_Contained_Block(const Self_Contained_Block& other) = delete;
        Self_Contained_Block(Self_Contained_Block&& other) = delete;

        char m_data[Size];
    };

    // Facilitates acquiring memory from the Heap.
    // Releases the memory on destruction (RAII).
    // NOTE: This structure defeats the purpose of the library.
    // NOTE: This auto acquired memory block uses new to get memory.
    template <std::size_t Size>
    struct Heap_Block : Block
    {
        Heap_Block()
            : m_data(std::make_unique<char>(Size))
            , Block{.base = m_data.get(), .size = Size}
        { }

        std::unique_ptr<char> m_data;
    };
}
