// test_any_block_allocator.cpp
#include <allocators/any_block_allocator.hpp>
#include <allocators/basic/buddy.hpp>
#include <allocators/basic/pool.hpp>
#include <allocators/basic/stack.hpp>
#include <cstdio>
#include <cstring>
#include <new>

using namespace dd99::memory;
using namespace dd99::memory::block_allocator;

// utility: aligned memory buffer
alignas(std::max_align_t) static std::byte buffer[64 * 1024];

// simple test harness
static unsigned passed = 0, failed = 0;

#define TEST(name) do { std::printf("  %s ... ", name); } while(0)
#define OK() do { std::printf("PASS\n"); ++passed; } while(0)
#define FAIL(msg) do { std::printf("FAIL: %s\n", msg); ++failed; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

// ====================================================================
// Tests
// ====================================================================

void test_basic_allocation_deallocation()
{
    TEST("allocate / deallocate via buddy");
    // Create a buddy allocator on the buffer
    Buddy<64, 5> buddy(Block{buffer, sizeof(buffer)});
    any_block_allocator_ref ref = buddy;

    // Allocate a 128-byte block
    Block b = ref.allocate(128);
    CHECK(b, "allocation returned empty block");
    CHECK(b.size >= 128, "allocated block size is too small");
    CHECK(ref.owns(b), "owns(Block) failed");
    CHECK(ref.owns(b.base), "owns(pointer) failed");

    // Deallocate it
    ref.deallocate(b);
    // Allocating again should succeed (same size)
    Block b2 = ref.allocate(128);
    CHECK(b2, "re-allocation after deallocate failed");
    // Might be the same address, but no guarantee; just check non‑empty.
    ref.deallocate(b2);
    OK();
}

void test_deallocate_all()
{
    TEST("deallocate_all");
    Buddy<64, 10> buddy(Block{buffer, sizeof(buffer)});
    any_block_allocator_ref ref = buddy;

    // Allocate several blocks
    Block a = ref.allocate(64);
    Block b = ref.allocate(64);
    CHECK(a && b, "initial allocations failed");

    ref.deallocate_all();

    // After deallocate_all, we should be able to allocate up to the maximum again.
    Block c = ref.allocate(4096); // large block that would need contiguous space
    CHECK(c, "large allocation after deallocate_all failed");
    ref.deallocate(c);
    OK();
}

void test_owns_semantics()
{
    TEST("owns checks");
    Buddy<64, 5> buddy(Block{buffer, sizeof(buffer)});
    any_block_allocator_ref ref = buddy;

    // A random stack pointer must not be owned
    std::byte stack_var;
    CHECK(!ref.owns(&stack_var), "should not own stack variable");

    Block b = ref.allocate(64);
    CHECK(b, "allocation failed");
    CHECK(ref.owns(b), "owns block should be true");
    CHECK(ref.owns(b.base), "owns pointer should be true");

    // Alter the end pointer to make it out of bounds -> should not own
    Block fake = b;
    fake.size += 1;
    CHECK(!ref.owns(fake), "oversized block should not be owned");

    ref.deallocate(b);
    OK();
}

void test_wrapper_copy()
{
    TEST("copy of wrapper");
    Buddy<64, 5> buddy(Block{buffer, sizeof(buffer)});
    any_block_allocator_ref ref1 = buddy;
    any_block_allocator_ref ref2 = ref1;   // copy

    // Both should work and point to the same allocator
    Block b1 = ref1.allocate(128);
    CHECK(b1, "first wrapper allocation failed");
    CHECK(ref2.owns(b1), "second wrapper does not own block from first");
    ref2.deallocate(b1);   // deallocate via second wrapper
    Block b2 = ref1.allocate(128); // should succeed now
    CHECK(b2, "re-allocation after deallocate via other wrapper failed");
    ref1.deallocate(b2);
    OK();
}

void test_type_erased_dispatch()
{
    TEST("type-erased dispatch (different allocators)");
    // Use a Buddy and a Pool, both wrapped, to ensure the correct vtable is used.
    Buddy<64, 4> buddy(Block{buffer, sizeof(buffer)/2});       // half of buffer
    Pool<128> pool(Block{buffer + sizeof(buffer)/2, sizeof(buffer)/2});

    any_block_allocator_ref ref_buddy = buddy;
    any_block_allocator_ref ref_pool = pool;

    auto b1 = ref_buddy.allocate(200);
    auto b2 = ref_pool.allocate(200);   // Pool<128> max allocation is 128, so 200 should fail -> empty
    CHECK(b1, "buddy allocation failed");
    CHECK(!b2, "pool allocation for 200 should have failed (block size 128)");

    // Check that owns reporting matches the original allocator
    CHECK(ref_buddy.owns(b1), "buddy should own its block");
    CHECK(!ref_pool.owns(b1), "pool should not own buddy's block");

    ref_buddy.deallocate(b1);
    OK();
}

void test_vtable_sharing()
{
    TEST("vtable pointer identity");
    // Two wrappers of the same type should have the same vtable pointer.
    Buddy<64, 5> b1(Block{buffer, sizeof(buffer)/2});
    Buddy<64, 5> b2(Block{buffer + sizeof(buffer)/2, sizeof(buffer)/2});
    any_block_allocator_ref r1 = b1;
    any_block_allocator_ref r2 = b2;
    CHECK(r1.m_vptr == r2.m_vptr, "vtable pointers differ for same concrete type");

    // Different type -> different vtable pointer
    Buddy<128, 3> b3(Block{buffer, sizeof(buffer)});
    any_block_allocator_ref r3 = b3;
    CHECK(r1.m_vptr != r3.m_vptr, "vtable pointers same for different types");
    OK();
}

void test_concept_satisfaction()
{
    TEST("Block_Allocator concept satisfied");
    // The static_assert inside the header already checks this.
    // Here we call a template function that requires Block_Allocator.
    auto use_alloc = [](Block_Allocator auto & a) {
        Block b = a.allocate(8);
        if (b) a.deallocate(b);
    };
    Buddy<64, 5> buddy(Block{buffer, sizeof(buffer)});
    any_block_allocator_ref ref = buddy;
    use_alloc(ref);   // must compile and run
    OK();
}

void test_move_semantics_in_wrapper()
{
    TEST("move of wrapper");
    Buddy<64, 5> buddy(Block{buffer, sizeof(buffer)});
    any_block_allocator_ref ref = buddy;
    auto ref2 = std::move(ref);
    // After move, ref2 should still work (ref is still valid, pointing to same allocator because it's just a value copy)
    Block b = ref2.allocate(64);
    CHECK(b, "allocation via moved wrapper failed");
    ref2.deallocate(b);
    OK();
}

// ====================================================================
// Main
// ====================================================================
int main()
{
    std::printf("Running any_block_allocator_ref tests...\n");

    test_basic_allocation_deallocation();
    test_deallocate_all();
    test_owns_semantics();
    test_wrapper_copy();
    test_type_erased_dispatch();
    test_vtable_sharing();
    test_concept_satisfaction();
    test_move_semantics_in_wrapper();

    std::printf("\n%u tests passed, %u tests failed.\n", passed, failed);
    return (failed > 0) ? 1 : 0;
}
