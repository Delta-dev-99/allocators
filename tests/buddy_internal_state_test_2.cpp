// buddy_internal_state_tests.cpp
#include <allocators/block_allocators/basic/buddy/buddy.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_layout.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_block_address.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_state.hpp>
#include <allocators/block_allocators/basic/buddy/state_implementations/buddy_intrusive_state.hpp>
#include "buddy_internal_test_helpers.hpp"   // your implemented helpers

#include <vector>
#include <cassert>
#include <iostream>
#include <algorithm>

using namespace dd99::memory;
using namespace dd99::memory::block_allocator;
using namespace dd99::memory::block_allocator::buddy_namespace;

// ---------------------------------------------------------------------------
// Configuration for the tests – adjust as needed
// ---------------------------------------------------------------------------
constexpr std::size_t BlockSize = 64;
constexpr std::size_t Levels = 5;               // max block = 64 * 2^(5-1) = 1024
using BlockAddr = buddy_block_address<>;
using Layout = buddy_standard_layout<BlockAddr, BlockSize, Levels>;
using State = buddy_intrusive_state<Layout, block>;

// ---------------------------------------------------------------------------
// Helper to create a state object with given managed memory size
// ---------------------------------------------------------------------------
std::pair<Layout, State> make_state(std::size_t managed_size) {
    std::vector<std::byte> mem(managed_size);
    Layout layout(block{mem.data(), managed_size});
    auto state_bytes = buddy_intrusive_state_traits<Layout>::get_state_size(layout);
    std::vector<std::byte> state_mem(state_bytes);
    State state(std::move(layout), block{state_mem.data(), state_bytes});
    // The layout is moved, but we kept a copy before? Actually layout is moved.
    // We need the layout later for inspection. So we'll return a copy of the original?
    // Better: construct layout, then copy it before moving.
    // We'll restructure:
}
// Correction: we need to preserve the layout for later queries.
// The state owns the layout; we can access it via state.m_layout.
// So we only need to create the state and use state.m_layout.

// ---------------------------------------------------------------------------
// 1. Initial state – full size (exact largest block)
// ---------------------------------------------------------------------------
void test_initial_full() {
    std::size_t managed = BlockSize * (1ULL << (Levels - 1)); // 1024
    std::vector<std::byte> mem(managed);
    Layout layout(block{mem.data(), managed});
    auto state_size = buddy_intrusive_state_traits<Layout>::get_state_size(layout);
    std::vector<std::byte> state_mem(state_size);
    State state(std::move(layout), block{state_mem.data(), state_size});
    const auto& L = state.m_layout;
    const auto& freearr = state.m_freelist_collection;
    const auto& bitmap = state.m_state_tracker.m_bitmap;

    // Last level: all blocks free
    auto last = Levels - 1;
    auto count_last = L.get_level_block_count(last);
    auto free_last = get_freelist_contents(freearr[last]);
    assert(free_last.size() == count_last);
    for (index_type i = 0; i < count_last; ++i) {
        auto expected = L.get_block({last, i}).base;
        assert(free_last[i] == expected);
    }

    // Levels 0 .. last-1: only blocks without a buddy are free
    for (auto lvl = 0; lvl < last; ++lvl) {
        auto cnt = L.get_level_block_count(lvl);
        auto free_lvl = get_freelist_contents(freearr[lvl]);
        if (cnt == 0) {
            assert(free_lvl.empty());
        } else {
            block_address_type last_blk{lvl, static_cast<index_type>(cnt - 1)};
            if (!L.block_has_buddy(last_blk)) {
                assert(free_lvl.size() == 1);
                assert(free_lvl[0] == L.get_block(last_blk).base);
            } else {
                assert(free_lvl.empty());
            }
        }
    }

    // Bitmap: all bits zero
    for (std::size_t i = 0; i < bitmap_size(bitmap); ++i)
        assert(!get_bit(bitmap, i));

    std::cout << "Initial full state OK\n";
}

// ---------------------------------------------------------------------------
// 2. Initial state – odd block count (e.g., 3 base blocks)
// ---------------------------------------------------------------------------
void test_initial_odd() {
    std::size_t managed = BlockSize * 3; // 192 bytes, 3 base blocks
    std::vector<std::byte> mem(managed);
    Layout layout(block{mem.data(), managed});
    auto state_size = buddy_intrusive_state_traits<Layout>::get_state_size(layout);
    std::vector<std::byte> state_mem(state_size);
    State state(std::move(layout), block{state_mem.data(), state_size});
    const auto& L = state.m_layout;
    const auto& freearr = state.m_freelist_collection;
    const auto& bitmap = state.m_state_tracker.m_bitmap;

    // Level 0: 3 blocks, index 2 has no buddy -> free
    auto free0 = get_freelist_contents(freearr[0]);
    assert(free0.size() == 1);
    assert(free0[0] == L.get_block({0, 2}).base);

    // Level 1: 1 block (joint of 0-1), no buddy -> free
    auto free1 = get_freelist_contents(freearr[1]);
    assert(free1.size() == 1);
    assert(free1[0] == L.get_block({1, 0}).base);

    // Higher levels empty
    for (auto l = 2; l < Levels; ++l)
        assert(get_freelist_contents(freearr[l]).empty());

    // Bitmap: 1 bit for level‑1 block, initially 0
    assert(bitmap_size(bitmap) == 1);
    assert(!get_bit(bitmap, 0));

    std::cout << "Initial odd state OK\n";
}

// ---------------------------------------------------------------------------
// 3. Allocation of a block one level below top (split scenario)
// ---------------------------------------------------------------------------
void test_alloc_split() {
    std::size_t managed = BlockSize * (1ULL << (Levels - 1)); // 1024
    std::vector<std::byte> mem(managed);
    Layout layout(block{mem.data(), managed});
    auto state_size = buddy_intrusive_state_traits<Layout>::get_state_size(layout);
    std::vector<std::byte> state_mem(state_size);
    State state(std::move(layout), block{state_mem.data(), state_size});
    auto& freearr = state.m_freelist_collection;
    auto& L = state.m_layout;
    auto& tracker = state.m_state_tracker;

    auto top = Levels - 1;          // 4
    auto req = top - 1;             // 3

    // --- Simulate allocate_impl(req):
    // 1. pop(top) – take the single top‑level block
    auto block_top = state.pop(top);
    assert(block_top != nullptr);

    // After pop, top freelist empty, bitmap unchanged (top block has no buddy).
    assert(get_freelist_contents(freearr[top]).empty());

    // 2. push buddy of requested level
    auto half = Layout::get_level_block_size(req);
    auto buddy = block_top + half;
    state.push(req, buddy);

    // Now freelist at req contains buddy
    auto free_req = get_freelist_contents(freearr[req]);
    assert(free_req.size() == 1);
    assert(free_req[0] == buddy);

    // Bitmap: the joint block covering the pair (req) is at level top.
    // block address of the allocated block? We need the address of the block we kept (left part).
    // That block is at level req, index 0 (since block_top is the base).
    block_address_type allocated_addr = L.get_block_address(block_top, req);
    block_address_type joint_addr = Layout::get_joint_block_address(allocated_addr);
    // Verify level and index
    assert(joint_addr.level == top);
    assert(joint_addr.index == 0);

    auto bit_idx = tracker.get_bitmap_index_from_joint(joint_addr, L);
    // After push, the bit toggles (one allocated, one free) -> must be 1
    assert(get_bit(tracker.m_bitmap, bit_idx) == true);

    // Clean up: pop the buddy back, push the top block to restore state (or just reset)
    state.reset(); // easiest for next tests
    std::cout << "Alloc split state OK\n";
}

// ---------------------------------------------------------------------------
// 4. Deallocation that merges with a free buddy
// ---------------------------------------------------------------------------
void test_dealloc_merge() {
    std::size_t managed = BlockSize * (1ULL << (Levels - 1));
    std::vector<std::byte> mem(managed);
    Layout layout(block{mem.data(), managed});
    auto state_size = buddy_intrusive_state_traits<Layout>::get_state_size(layout);
    std::vector<std::byte> state_mem(state_size);
    State state(std::move(layout), block{state_mem.data(), state_size});
    auto& freearr = state.m_freelist_collection;
    auto& L = state.m_layout;
    auto& tracker = state.m_state_tracker;

    // Obtain an allocated block at level 0 by popping (forces splits)
    auto allocated = state.pop(0);
    assert(allocated != nullptr);

    // The level‑0 freelist now contains its buddy (pushed during split)
    auto free0_list = get_freelist_contents(freearr[0]);
    assert(free0_list.size() == 1);
    auto buddy = free0_list[0];

    // Bitmap: the joint block (level 1) for this pair should be 1 (different states)
    block_address_type alloc_addr = L.get_block_address(allocated, 0);
    block_address_type joint_l1 = Layout::get_joint_block_address(alloc_addr);
    auto bit_idx = tracker.get_bitmap_index_from_joint(joint_l1, L);
    assert(get_bit(tracker.m_bitmap, bit_idx) == true);

    // Now deallocate the allocated block via merge_or_push
    auto merged_base = state.merge_or_push(0, allocated);
    // Since buddy was free, it should merge and return the joint block's base
    assert(merged_base != nullptr);
    assert(merged_base == std::min(allocated, buddy));

    // Level‑0 freelist must be empty (both blocks removed)
    assert(get_freelist_contents(freearr[0]).empty());

    // Bitmap bit toggled back to 0
    assert(get_bit(tracker.m_bitmap, bit_idx) == false);

    // The joint block itself (level 1) is now free. We should push it to continue the recursion,
    // but our test stops here because merge_or_push returned the base for that purpose.
    // In a full deallocation, the caller would then call merge_or_push on level 1 with merged_base.
    state.reset();
    std::cout << "Dealloc merge state OK\n";
}

// ---------------------------------------------------------------------------
// 5. Operations on blocks without a buddy (no bitmap involved)
// ---------------------------------------------------------------------------
void test_no_buddy_blocks() {
    // Use odd size again – level 0 index 2 has no buddy
    std::size_t managed = BlockSize * 3;
    std::vector<std::byte> mem(managed);
    Layout layout(block{mem.data(), managed});
    auto state_size = buddy_intrusive_state_traits<Layout>::get_state_size(layout);
    std::vector<std::byte> state_mem(state_size);
    State state(std::move(layout), block{state_mem.data(), state_size});
    auto& freearr = state.m_freelist_collection;
    auto& L = state.m_layout;

    // Level 0, index 2 is free and has no buddy
    auto blk_addr = block_address_type{0, 2};
    auto blk_base = L.get_block(blk_addr).base;

    // Initially freelist contains it
    auto free0 = get_freelist_contents(freearr[0]);
    assert(free0.size() == 1 && free0[0] == blk_base);

    // pop it
    auto popped = state.pop(0);
    assert(popped == blk_base);
    // Freelist empty, bitmap unchanged (no joint bit for this block)
    assert(get_freelist_contents(freearr[0]).empty());

    // push it back
    state.push(0, blk_base);
    free0 = get_freelist_contents(freearr[0]);
    assert(free0.size() == 1 && free0[0] == blk_base);

    // merge_or_push on it: no buddy -> pushed, returns nullptr
    auto merged = state.merge_or_push(0, blk_base);
    assert(merged == nullptr);
    // Still in freelist
    free0 = get_freelist_contents(freearr[0]);
    assert(free0.size() == 1 && free0[0] == blk_base);

    std::cout << "No‑buddy blocks OK\n";
}

// ---------------------------------------------------------------------------
// 6. Bitmap index calculation for multiple levels
// ---------------------------------------------------------------------------
void test_bitmap_indices() {
    // Use full size, walk through cumulative indices
    std::size_t managed = BlockSize * (1ULL << (Levels - 1));
    std::vector<std::byte> mem(managed);
    Layout layout(block{mem.data(), managed});
    auto state_size = buddy_intrusive_state_traits<Layout>::get_state_size(layout);
    std::vector<std::byte> state_mem(state_size);
    State state(std::move(layout), block{state_mem.data(), state_size});
    const auto& L = state.m_layout;
    const auto& tracker = state.m_state_tracker;

    // Joint blocks exist for levels 1..last_level? Actually levels 1 to last_level are joint.
    // Number of bits = sum over level=1..last_level of count(level). But layout's get_total_joint_block_count
    // uses for(level=1; level<levels; ++level) i.e. levels 1..last_level-1? Let's check:
    // levels is total number of levels (5). last_level = levels-1 = 4.
    // Loop for level=1; level<levels; ++level covers 1,2,3,4. So all levels >=1.
    // That matches the fact that level 0 has no joint block above it.
    // The bitmap stores bits for every block on levels 1..last_level.
    // The index of a joint block at (level, index) is:
    //   cumulative_joint_block_count[level-1] + index
    // This is exactly what get_bitmap_index_from_joint does.
    // We'll test a few addresses.
    auto last = Levels - 1;
    // Level 1, index 0
    auto addr10 = block_address_type{1, 0};
    auto idx10 = tracker.get_bitmap_index_from_joint(addr10, L);
    assert(idx10 == L.get_cumulative_joint_block_count(0) + 0); // cumulative for level 0 is 0
    // Level 2, index 0
    auto addr20 = block_address_type{2, 0};
    auto idx20 = tracker.get_bitmap_index_from_joint(addr20, L);
    assert(idx20 == L.get_cumulative_joint_block_count(1) + 0);
    // Level last, index 0
    auto addr_last = block_address_type{last, 0};
    auto idx_last = tracker.get_bitmap_index_from_joint(addr_last, L);
    assert(idx_last == L.get_cumulative_joint_block_count(last-1) + 0);

    // Check that total bitmap size matches
    assert(bitmap_size(tracker.m_bitmap) == L.get_total_joint_block_count());
    std::cout << "Bitmap index calculations OK\n";
}

// ---------------------------------------------------------------------------
int main() {
    test_initial_full();
    test_initial_odd();
    test_alloc_split();
    test_dealloc_merge();
    test_no_buddy_blocks();
    test_bitmap_indices();
    std::cout << "All internal state tests passed.\n";
    return 0;
}