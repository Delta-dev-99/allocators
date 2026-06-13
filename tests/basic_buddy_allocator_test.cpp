// #include <allocators/block_allocators/basic/buddy/buddy.hpp>   // adjust path as needed
// #include <allocators/structures/blocks/memory_block.hpp>

// #include <cstdint>
// #include <cstdio>         // printf for output
// #include <cstring>        // memset
// #include <utility>        // std::move
// #include <cstdlib>

// // Test configuration
// constexpr std::size_t BLOCK_SIZE = 64;
// constexpr unsigned LEVELS = 5;                // max block = 64 << 4 = 1024
// constexpr std::size_t MEM_SIZE = 1024 * 64;   // 64 KiB

// // Allocator type
// using BuddyAlloc = dd99::memory::block_allocator::Buddy<BLOCK_SIZE, LEVELS>;

// // Simple linear allocator for tracking allocated blocks in test
// struct AllocRecord {
//     dd99::memory::block block;
//     bool used = false;
// };

// // ------------------------------------------------------------------
// // Helper to check that a block is within memory
// // ------------------------------------------------------------------
// bool block_within(dd99::memory::block b, std::byte* mem, std::size_t size) {
//     return b.base >= mem && (b.base + b.size) <= (mem + size);
// }

// // ------------------------------------------------------------------
// // Helper to check no overlapping allocations
// // ------------------------------------------------------------------
// bool no_overlap(const AllocRecord* records, std::size_t count) {
//     for (std::size_t i = 0; i < count; ++i) {
//         if (!records[i].used) continue;
//         auto& a = records[i].block;
//         for (std::size_t j = i + 1; j < count; ++j) {
//             if (!records[j].used) continue;
//             auto& b = records[j].block;
//             // Check for intersection
//             if (!(a.get_end() <= b.base || b.get_end() <= a.base))
//                 return false;
//         }
//     }
//     return true;
// }

// // ------------------------------------------------------------------
// // Main test routine
// // ------------------------------------------------------------------
int main() {
//     std::printf("Buddy allocator test: BLOCK_SIZE=%zu, LEVELS=%u, MEM_SIZE=%zu\n",
//                 BLOCK_SIZE, LEVELS, MEM_SIZE);

//     // Provide memory for the allocator
//     static std::byte mem[MEM_SIZE];
//     std::memset(mem, 0, sizeof(mem));   // optional

//     // Build the buddy allocator on this memory.
//     // The constructor will carve out the bitmap and determine block count.
//     BuddyAlloc buddy(dd99::memory::block{mem, MEM_SIZE});

//     // Verify that calculate_block_count matches the actual usable blocks.
//     std::size_t expected_block_count = BuddyAlloc::calculate_block_count(MEM_SIZE);
//     std::printf("calculate_block_count = %zu\n", expected_block_count);

//     // We'll allocate many blocks and record them.
//     constexpr std::size_t MAX_ALLOCS = 2 * 1024; // reallocation test duplicates entries
//     AllocRecord records[MAX_ALLOCS]{};
//     std::size_t alloc_count = 0;

//     auto store_alloc = [&](dd99::memory::block blk) {
//         if (!blk) return;
//         if (alloc_count >= MAX_ALLOCS) {
//             std::printf("FAIL: Too many allocations\n");
//             std::exit(1);
//         }
//         records[alloc_count++] = {blk, true};
//     };

//     // ------------------------------------------------------------------
//     // 1. Test allocation of all possible sizes (smallest to largest)
//     // ------------------------------------------------------------------
//     std::printf("Test: allocate blocks of various sizes...\n");
//     bool ok = true;
//     for (std::size_t sz = 1; sz <= (BLOCK_SIZE << (LEVELS-1)); sz *= 2) {
//         if (sz > (BLOCK_SIZE << (LEVELS-1))) break;
//         // Allocate several blocks of this size
//         for (int i = 0; i < 4; ++i) {
//             dd99::memory::block blk = buddy.allocate(sz);
//             if (blk) {
//                 if (!block_within(blk, mem, MEM_SIZE)) {
//                     std::printf("FAIL: allocated block outside memory\n");
//                     ok = false;
//                 }
//                 if (blk.size < sz) {
//                     std::printf("FAIL: allocated block with wrong size\n");
//                     ok = false;
//                 }
//                 store_alloc(blk);
//             }
//         }
//     }
//     if (ok) std::printf("  ok\n");

//     // Check no overlap
//     if (!no_overlap(records, alloc_count)) {
//         std::printf("FAIL: overlapping allocations detected\n");
//         return 1;
//     }
//     std::printf("No overlap check passed.\n");

//     // ------------------------------------------------------------------
//     // 2. Test exhaustion: allocate until failure, then check total count
//     // ------------------------------------------------------------------
//     std::printf("Test: exhaust memory...\n");
//     dd99::memory::block blk;
//     while ((blk = buddy.allocate(64))) {
//         store_alloc(blk);
//     }
//     std::printf("Allocated %zu blocks until exhaustion.\n", alloc_count);
//     if (!no_overlap(records, alloc_count)) {
//         std::printf("FAIL: overlapping after exhaustion\n");
//         return 1;
//     }

//     // Now a further allocation should return empty
//     blk = buddy.allocate(1);
//     if (blk) {
//         std::printf("FAIL: allocation succeeded when memory should be full\n");
//         return 1;
//     }
//     std::printf("Exhaustion check passed.\n");

//     // ------------------------------------------------------------------
//     // 3. Deallocation and reallocation
//     // ------------------------------------------------------------------
//     std::printf("Test: deallocate some blocks and reallocate...\n");
//     // Free half of the recorded blocks
//     std::size_t deallocated = 0;
//     for (std::size_t i = 0; i < alloc_count / 2; ++i) {
//         buddy.deallocate(records[i].block);
//         records[i].used = false;
//         ++deallocated;
//     }
//     // Now allocate again – should succeed
//     std::size_t reallocated = 0;
//     for (std::size_t i = 0; i < alloc_count / 2; ++i) {
//         dd99::memory::block blk2 = buddy.allocate(64);
//         if (!blk2) break;
//         store_alloc(blk2);
//         ++reallocated;
//     }
//     std::printf("Reallocated %zu blocks.\n", reallocated);
//     if (reallocated == 0) {
//         std::printf("FAIL: could not reallocate after freeing\n");
//         return 1;
//     }
//     if (!no_overlap(records, alloc_count)) {
//         std::printf("FAIL: overlap after reallocation\n");
//         return 1;
//     }

//     // ------------------------------------------------------------------
//     // 4. Buddy merging test: free a pair of buddies, then allocate larger block
//     // ------------------------------------------------------------------
//     std::printf("Test: buddy merging...\n");
//     // Free all blocks first
//     buddy.deallocate_all();
//     alloc_count = 0;

//     // Allocate two small blocks that should be buddies
//     auto a1 = buddy.allocate(64);
//     auto a2 = buddy.allocate(64);
//     if (!a1 || !a2) {
//         std::printf("FAIL: could not allocate two blocks for buddy test\n");
//         return 1;
//     }
//     // Check they are adjacent? Not necessary; we'll trust the allocator.
//     // Free both — they should merge into a block of size 128 (if L>=2)
//     buddy.deallocate(a1);
//     buddy.deallocate(a2);
//     // Now allocate a 128‑byte block (should succeed)
//     auto a3 = buddy.allocate(128);
//     if (!a3) {
//         std::printf("FAIL: buddy merge failed; cannot allocate 128 after freeing two 64s\n");
//         return 1;
//     }
//     if (a3.base != std::min(a1.base, a2.base)) {
//         std::printf("FAIL: buddy merge failed; resulting allocation wasn't the combination of the smaller blocks\n");
//         return 1;
//     }
//     buddy.deallocate(a3);
//     std::printf("Buddy merge test passed.\n");

//     // ------------------------------------------------------------------
//     // 5. Allocate maximum block size
//     // ------------------------------------------------------------------
//     std::printf("Test: allocate maximum block size...\n");
//     auto max_sz = BLOCK_SIZE << (LEVELS-1);
//     auto big = buddy.allocate(max_sz);
//     if (!big) {
//         std::printf("FAIL: cannot allocate maximum block size %zu\n", max_sz);
//         return 1;
//     }
//     buddy.deallocate(big);
//     std::printf("Maximum block allocation successful.\n");

//     // ------------------------------------------------------------------
//     // 6. Test that deallocation of unowned block does nothing (should not crash)
//     // ------------------------------------------------------------------
//     std::printf("Test: deallocate unowned pointer...\n");
//     std::byte alien[BLOCK_SIZE];
//     buddy.deallocate({alien, BLOCK_SIZE}); // should be no-op (owns returns false)
//     std::printf("No crash on unowned deallocation.\n");

//     // ------------------------------------------------------------------
//     // 7. Zero-size allocation edge case
//     // ------------------------------------------------------------------
//     std::printf("Test: zero-size allocation...\n");
//     auto zero = buddy.allocate(0);
//     if (zero) {
//         std::printf("FAIL: zero-size allocation should return empty block\n");
//         return 1;
//     }
//     std::printf("Zero-size correctly rejected.\n");

//     // ------------------------------------------------------------------
//     // 8. Confirm block count matches expected
//     // ------------------------------------------------------------------
//     std::printf("Test: block count consistency...\n");
//     // Reinitialize allocator to get fresh counts
//     BuddyAlloc buddy2(dd99::memory::block{mem, MEM_SIZE});
//     std::size_t blocks_used = 0;
//     while (true) {
//         auto b = buddy2.allocate(BLOCK_SIZE);
//         if (!b) break;
//         ++blocks_used;
//     }
//     // The maximum number of lowest‑level blocks we can allocate is what calculate_block_count returns.
//     if (blocks_used != expected_block_count) {
//         std::printf("FAIL: expected %zu blocks, could only allocate %zu\n",
//                     expected_block_count, blocks_used);
//         return 1;
//     }
//     std::printf("Block count matches calculation: %zu\n", blocks_used);

//     std::printf("\nAll tests passed.\n");
//     return 0;
}