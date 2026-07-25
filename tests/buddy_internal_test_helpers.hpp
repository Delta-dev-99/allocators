// buddy_internal_test_helpers.hpp
#pragma once

#include <vector>
#include <cstddef>
#include <allocators/structures/linked_list.hpp>   // your linked list
#include <allocators/structures/bitmap.hpp>        // your bitmap

namespace buddy_test {

// -----------------------------------------------------------------------
// Freelist inspection – assumes intrusive linked list stores nodes at the
// start of each free block. Replace with real iteration code.
// -----------------------------------------------------------------------
// template <typename FreelistType>
std::vector<std::byte*> get_freelist_contents(const dd99_allocators_namespace::structure::basic_linked_list & freelist) {
    std::vector<std::byte*> result;

    auto ptr = freelist.m_first_ptr;
    while (ptr != nullptr)
    {
        result.push_back(reinterpret_cast<std::byte *>(ptr));
        ptr = ptr->m_next_ptr;
    }

    return result;
}

// -----------------------------------------------------------------------
// Bitmap query – assumes Bitmap has `test(std::size_t index)` returning bool
// -----------------------------------------------------------------------
bool get_bit(const dd99_allocators_namespace::structure::Bitmap<std::byte> & bitmap, std::size_t index) {
    return bitmap[index];
}

std::size_t bitmap_size(const dd99_allocators_namespace::structure::Bitmap<std::byte> & bitmap) {
    return bitmap.bit_size();
}

} // namespace buddy_test