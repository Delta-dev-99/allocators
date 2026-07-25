// buddy_internal_state_test.cpp
#include <allocators/block_allocators/basic/buddy/buddy.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_layout.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_block_address.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_state.hpp>
#include <allocators/block_allocators/basic/buddy/state_implementations/buddy_intrusive_state.hpp>
#include <allocators/structures/blocks/memory_block.hpp>
#include <allocators/structures/blocks/raii_block.hpp>

#include <vector>
#include <cassert>
#include <iostream>
#include <algorithm>

#include "buddy_internal_test_helpers.hpp" // the helpers above

using namespace dd99_allocators_namespace;
using namespace dd99_allocators_namespace::block_allocator;
using namespace dd99_allocators_namespace::block_allocator::buddy_namespace;

// ---------------------------------------------------------------------------
// Test parameters
// ---------------------------------------------------------------------------
template <std::size_t BlockSize, std::size_t Levels, class LevelType = unsigned int, class IndexType = unsigned int>
struct TestConfig {
    using block_addr_type = buddy_block_address<LevelType, IndexType>;
    using layout_traits_type = buddy_standard_layout_traits<block_addr_type, BlockSize, Levels>;
    template <class Layout> using state_traits_type = buddy_intrusive_state_traits<Layout>;
    static constexpr auto levels = Levels;
    static constexpr auto block_size = BlockSize;
};

template <class Config>
auto make_buddy_state(std::size_t managed_size)
{
    // allocate managed memory and create layout
    constexpr auto alignment = Config::layout_traits_type::required_alignment;
    auto ptr = ::operator new(managed_size, std::align_val_t{alignment});
    if (!ptr) throw std::bad_alloc();
    auto b_ptr = reinterpret_cast<std::byte *>(ptr);
    auto blk = dd99_allocators_namespace::raii_block{{b_ptr, managed_size}, [](block blk){ ::operator delete(blk.base, std::align_val_t{alignment}); }};
    auto layout = Config::layout_traits_type::make_layout(std::move(blk));

    // allocate state memory and create state
    using state_traits_type = Config::template state_traits_type<decltype(layout)>;
    auto state_size = state_traits_type::get_state_size(layout);
    constexpr auto state_alignment = state_traits_type::get_state_alignment();
    auto state_ptr = ::operator new(state_size, std::align_val_t{state_alignment});
    if (!state_ptr) throw std::bad_alloc();
    auto state_b_ptr = reinterpret_cast<std::byte *>(state_ptr);
    auto state_blk = dd99_allocators_namespace::raii_block{{state_b_ptr, state_size}, [](block blk){ ::operator delete(blk.base, std::align_val_t{state_alignment}); }};
    return state_traits_type::make_state(std::move(layout), std::move(state_blk));
}


// ---------------------------------------------------------------------------
// Creation test for different memory sizes
// ---------------------------------------------------------------------------
template <typename Config>
void test_initial_state(std::size_t managed_size) {
    // using Layout = typename Config::Layout;
    // using State = typename Config::State;

    // std::vector<std::byte> mem(managed_size + Layout::last_level_alignment);
    // Layout layout(block{dd99_allocators_namespace::align_up(mem.data(), Layout::last_level_alignment), managed_size});
    // // state memory size = bitmap size
    // auto state_size = buddy_intrusive_state_traits<Layout>::get_state_size(layout);
    // std::vector<std::byte> state_mem(state_size);
    // State state(std::move(layout), block{state_mem.data(), state_size});

    auto state = make_buddy_state<Config>(managed_size);
    using Layout = decltype(state)::layout_type;

    // ---- Check freelists ----
    auto& layout_ref = state.m_layout; // public
    auto& freearr = state.m_freelist_collection; // public array

    // Last level: all blocks are free (no buddy pair needed)
    auto last = Config::levels - 1;
    auto last_count = layout_ref.get_level_block_count(last);
    auto free_last = buddy_test::get_freelist_contents(freearr[last]);
    assert(free_last.size() == last_count);

    // Lower levels: only blocks without a buddy (odd index at the end) are added
    for (typename Layout::level_type lvl = 0; lvl < static_cast<int>(last); ++lvl) {
        auto cnt = layout_ref.get_level_block_count(lvl);
        auto free_lvl = buddy_test::get_freelist_contents(freearr[lvl]);
        if (cnt == 0) {
            assert(free_lvl.empty());
        } else {
            // Check if last block has no buddy
            typename Layout::block_address_type last_blk{lvl, static_cast<typename Layout::index_type>(cnt - 1)};
            if (!layout_ref.block_has_buddy(last_blk)) {
                assert(free_lvl.size() == 1);
                // The free block should be the last one
                auto expected_base = layout_ref.get_block(last_blk).base;
                assert(free_lvl[0] == expected_base);
            } else {
                // If all blocks have buddies, no initial free blocks at this level
                assert(free_lvl.empty());
            }
        }
    }

    // ---- Check bitmap: all bits zero initially ----
    auto & bitmap = state.m_state_tracker.m_bitmap;
    for (std::size_t i = 0; i < buddy_test::bitmap_size(bitmap); ++i) {
        assert(!buddy_test::get_bit(bitmap, i));
    }

    std::cout << "Initial state OK for size=" << managed_size << "\n";
}

// ---------------------------------------------------------------------------
// Allocation & split state check
// ---------------------------------------------------------------------------
template <typename Config>
void test_alloc_split_state() {
    // using Layout = typename Config::Layout;
    // using State = typename Config::State;
    constexpr auto block_sz = Config::block_size;
    constexpr auto levels = Config::levels;

    // // Use a memory size that is exactly one maximum block (block_size << (levels-1))
    std::size_t managed_size = block_sz * (std::size_t{1} << (levels - 1));
    // std::vector<std::byte> mem(managed_size);
    // Layout layout(block{mem.data(), managed_size});
    // auto state_size = buddy_intrusive_state_traits<Layout>::get_state_size(layout);
    // std::vector<std::byte> state_mem(state_size);
    // State state(std::move(layout), block{state_mem.data(), state_size});

    auto state = make_buddy_state<Config>(managed_size);
    using Layout = decltype(state)::layout_type;

    auto& freearr = state.m_freelist_collection;
    auto& tracker = state.m_state_tracker;
    auto& layout_ref = state.m_layout;

    // Initially the maximum block (top level) is the only free block.
    auto top_level = levels - 1;
    assert(!freearr[top_level].empty());
    // No other freelist has blocks (except possibly unpaired lower? Here full size, all paired)

    // Allocate a block one level below top (i.e., half size)
    typename Layout::level_type req_level = top_level - 1;
    auto block_ptr = state.pop(top_level);   // simulate allocate_impl start: try pop top level
    assert(block_ptr != nullptr);
    // After popping the top-level block, its joint bit (if it had a buddy) would have toggled.
    // Here top level is the highest, so block_has_buddy returns false (top >= last_level).
    // So no bitmap change.
    // Now split: push buddy to req_level
    auto half_size = Layout::get_level_block_size(req_level);
    auto buddy_ptr = block_ptr + half_size;
    state.push(req_level, buddy_ptr);

    // Check freelist at req_level now contains buddy
    auto free_req = buddy_test::get_freelist_contents(freearr[req_level]);
    assert(free_req.size() == 1);
    assert(free_req[0] == buddy_ptr);

    // Check bitmap: the joint block for the pair (req_level+1 = top_level) should have bit = 1
    // Because one subblock (buddy) is free, the other (block_ptr) is allocated (not in any list)
    // auto joint_addr = Layout::get_joint_block_address(Layout::get_block_address(layout_ref.m_memory.base, block_ptr, top_level)); // actually we need block address of the allocated block at top level? Wait, joint block of the pair at req_level is at top_level.
    auto joint_addr = Layout::get_joint_block_address(layout_ref.get_block_address(block_ptr, req_level));
    // Actually the joint block address for the pair (two blocks at req_level) is at level = req_level+1 = top_level.
    // auto joint_idx = Layout::get_joint_block_address({req_level, 0}); // index 0 pair
    auto bit_index = tracker.get_bitmap_index_from_joint(joint_addr, layout_ref);
    assert(buddy_test::get_bit(tracker.m_bitmap, bit_index) == true);

    // Now allocate again from req_level: pop should return buddy_ptr
    auto popped = state.pop(req_level);
    assert(popped == buddy_ptr);
    // After pop, the joint bit toggles back to 0
    assert(!buddy_test::get_bit(tracker.m_bitmap, bit_index));
    // Freelist at req_level empty
    assert(buddy_test::get_freelist_contents(freearr[req_level]).empty());

    // Clean up: push both back
    state.push(req_level, buddy_ptr);
    state.push(req_level, block_ptr); // to be realistic, deallocate will do merge_or_push; but just reset for next tests
    state.reset();

    std::cout << "Alloc/split state OK\n";
}

// ---------------------------------------------------------------------------
// Deallocation & merge state check
// ---------------------------------------------------------------------------
template <typename Config>
void test_dealloc_merge_state() {
    // NOTE: this function is commented because it was AI-generated goop

    // // using Layout = typename Config::Layout;
    // // using State = typename Config::State;
    // constexpr auto block_sz = Config::block_size;
    // constexpr auto levels = Config::levels;
    // std::size_t managed_size = block_sz * (std::size_t{1} << (levels - 1));
    // // std::vector<std::byte> mem(managed_size);
    // // Layout layout(block{mem.data(), managed_size});
    // // auto state_size = buddy_intrusive_state_traits<Layout>::get_state_size(layout);
    // // std::vector<std::byte> state_mem(state_size);
    // // State state(std::move(layout), block{state_mem.data(), state_size});

    // auto state = make_buddy_state<Config>(managed_size);
    // using Layout = decltype(state)::layout_type;
    // auto mem_ptr = state.m_layout.m_memory.get_base();

    // auto& freearr = state.m_freelist_collection;
    // auto& tracker = state.m_state_tracker;
    // auto& layout_ref = state.m_layout;

    // // Start with two allocated buddy blocks at level 0.
    // auto lvl0 = 0;
    // auto blk0 = state.pop(lvl0);   // should exist because initially all last-level blocks? Wait, last level is top_level, not 0. We need to split down from top.
    // // Simpler: manually construct a situation with two adjacent free blocks at level 0, then allocate both.
    // // Let's split from top until level 0.
    // auto top = levels - 1;
    // auto blk_top = state.pop(top);
    // // split down to level 0
    // for (auto l = top; l-- > 0;) {
    //     state.push(l, blk_top + Layout::get_level_block_size(l));
    // }
    // // Now at level 0 we have one block free (the one not split? Wait, splitting: we kept left, pushed right. So left is allocated? Actually we popped top, then for each split we pushed the right buddy. So after pushing at each level, the leftmost block is not in any freelist (it is 'allocated'), and the rightmost buddies are in freelists. But we need two adjacent free blocks at level 0. Let's instead deallocate the left part? Or just use the state directly: push two adjacent blocks manually.
    // state.reset();
    // // After reset, all blocks are free as per initial state.
    // // For a full-size memory, level 0 has all base blocks free? Wait initial state only adds last level blocks and unpaired lower blocks. Level 0 blocks are all in buddy pairs, so initially freelist at level 0 is empty (all paired). So we can allocate two adjacent blocks at level 0 by calling pop on level 0, which will force splitting from higher levels.
    // // But easier: we can manually push two adjacent level-0 blocks to force them into the freelist, then test merge.
    // // Let's manually push them (simulate free of two blocks).
    // auto blk0_addr = Layout::get_block_address(layout_ref.m_memory.base, mem_ptr, 0);
    // auto blk1_addr = blk0_addr;
    // blk1_addr.index = 1; // adjacent
    // state.push(0, layout_ref.get_block(blk0_addr).base);
    // state.push(0, layout_ref.get_block(blk1_addr).base);
    // // Now both are in level-0 freelist, bitmap bit for joint (level 1, index 0) should be 0 (same state)
    // auto joint_l1 = Layout::get_joint_block_address(blk0_addr);
    // auto bit_idx = tracker.get_bitmap_index_from_joint(joint_l1, layout_ref);
    // assert(!buddy_test::get_bit(tracker.m_bitmap, bit_idx));

    // // Deallocate (merge_or_push) one of them
    // auto merged = state.merge_or_push(0, layout_ref.get_block(blk0_addr).base);
    // // Since both were free, after toggling bit (0->1), is_buddy_free = false, so no merge. Wait, earlier reasoning: if both free, bit 0. merge_or_push toggles -> 1, is_buddy_free = false, so it just pushes. That's double free? No, we had both free already. To test merge, we need one allocated, one free.
    // // So allocate one block first.
    // state.reset();
    // // Split from top to get one level-0 block allocated.
    // auto allocated_ptr = state.pop(0); // after sufficient splits
    // assert(allocated_ptr != nullptr);
    // // Now level-0 freelist contains the buddy block.
    // auto free_list0 = buddy_test::get_freelist_contents(freearr[0]);
    // assert(free_list0.size() == 1);
    // auto buddy_ptr = free_list0[0];
    // // Bitmap: joint bit should be 1 (different states)
    // auto blk0_addr2 = Layout::get_block_address(layout_ref.m_memory.base, allocated_ptr, 0);
    // auto joint_l1_2 = Layout::get_joint_block_address(blk0_addr2);
    // auto bit_idx2 = tracker.get_bitmap_index_from_joint(joint_l1_2, layout_ref);
    // assert(buddy_test::get_bit(tracker.m_bitmap, bit_idx2) == true);

    // // Now deallocate the allocated block (merge_or_push)
    // auto higher = state.merge_or_push(0, allocated_ptr);
    // // It should detect buddy free (bit toggles to 0, is_buddy_free true) and return the joint base.
    // assert(higher != nullptr);
    // assert(higher == std::min(allocated_ptr, buddy_ptr));
    // // Freelist at level 0 should be empty (both blocks removed)
    // assert(buddy_test::get_freelist_contents(freearr[0]).empty());
    // // Bitmap bit back to 0
    // assert(!buddy_test::get_bit(tracker.m_bitmap, bit_idx2));
    // // The joint block is now being returned for further merging (recursive step). We can stop here.

    // std::cout << "Dealloc/merge state OK\n";
}

// ---------------------------------------------------------------------------
// Edge: odd block count where last block has no buddy
// ---------------------------------------------------------------------------
template <typename Config>
void test_odd_block_count_internal() {
    // using Layout = typename Config::Layout;
    // using State = typename Config::State;
    // // Managed size = block_size * (some odd count)
    constexpr auto bsz = Config::block_size;
    std::size_t odd_count = 3; // 3 base blocks
    std::size_t managed_size = bsz * odd_count;
    // std::vector<std::byte> mem(managed_size);
    // Layout layout(block{mem.data(), managed_size});
    // auto state_size = buddy_intrusive_state_traits<Layout>::get_state_size(layout);
    // std::vector<std::byte> state_mem(state_size);
    // State state(std::move(layout), block{state_mem.data(), state_size});

    auto state = make_buddy_state<Config>(managed_size);

    auto& freearr = state.m_freelist_collection;
    auto& layout_ref = state.m_layout;

    // Level 0: 3 blocks, index 2 has no buddy -> should be in freelist
    auto free0 = buddy_test::get_freelist_contents(freearr[0]);
    assert(free0.size() == 1);
    auto blk2 = layout_ref.get_block({0, 2});
    assert(free0[0] == blk2.base);

    // Level 1: 1 block (joint of blocks 0-1), index 0 has no buddy -> should be free
    auto free1 = buddy_test::get_freelist_contents(freearr[1]);
    assert(free1.size() == 1);
    auto blk_l1 = layout_ref.get_block({1, 0});
    assert(free1[0] == blk_l1.base);

    // All other levels empty
    for (int l = 2; l < Config::levels; ++l)
        assert(buddy_test::get_freelist_contents(freearr[l]).empty());

    // Bitmap: only level 1 joint bit? Level 1 has a joint? Wait, level 1 block itself is a block, and if it had a buddy we'd have a joint at level 2, but here level 1 count = 1, so no buddy, so no bitmap bit for it. All bits remain 0.
    // There's a joint bit for the pair (0,1) at level 1? Actually the joint block address for level 0 pair (0,1) is level 1, index 0. That joint block itself might have a buddy if there were another joint block at level 1. Since only one level-1 block, no buddy. The bitmap stores bits for joint blocks from level 1 to last_level? Wait, `get_total_joint_block_count` sums blocks from level=1 to last_level-1? Actually it iterates `for (level=1; level < levels; ++level)`, so includes level 1 blocks up to level last_level-1? Let's check: `levels` is the total number of levels (0..levels-1). So blocks on level >=1 are "joint" blocks. So the bitmap has one bit for each block on levels >=1. So for our case, level 1 block count = 1, so bitmap has 1 bit. Initially 0. That's fine.

    std::cout << "Odd block count internal state OK\n";
}

// ---------------------------------------------------------------------------
// Run all internal tests
// ---------------------------------------------------------------------------
int main() {
    using Config = TestConfig<64, 5>; // block_size 64, 5 levels (max block 1024)

    test_initial_state<Config>(64 * 16); // full size 1024
    test_initial_state<Config>(64 * 9);  // odd block count (9 base blocks)

    test_alloc_split_state<Config>();
    test_dealloc_merge_state<Config>();
    test_odd_block_count_internal<Config>();

    std::cout << "All internal state tests passed.\n";
    return 0;
}
