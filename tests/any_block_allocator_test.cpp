// test_any_block_allocator.cpp
#include <allocators/block_allocators/any_block_allocator.hpp>
#include <allocators/block_allocators/basic/buddy/buddy.hpp>
#include <allocators/block_allocators/basic/buddy/state_implementations/buddy_intrusive_state.hpp>
#include <allocators/structures/blocks/raii_block.hpp>
// #include <allocators/block_allocators/basic/pool.hpp> // TODO: enable
// #include <allocators/block_allocators/basic/stack.hpp> // TODO: enable
#include <cstdio>
#include <cstring>
#include <array>
#include <memory>
#include <new>

using namespace dd99::memory;
using namespace dd99::memory::block_allocator;

// utility: aligned memory buffer
// alignas(std::max_align_t) static std::byte buffer[64 * 1024];
alignas(32 * 1024) static std::byte buffer[64 * 1024]; // alignment required by buddy with 64-byte blocks and up to 5 levels (max block size 2048 bytes) - ensures the entire buffer can be used without alignment issues

// simple test harness
static unsigned passed = 0, failed = 0;

#define TEST(name) do { std::printf("  %s ... ", name); } while(0)
#define OK() do { std::printf("PASS\n"); ++passed; } while(0)
#define FAIL(msg) do { std::printf("FAIL: %s\n", msg); ++failed; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)


// create a buddy allocator using intrusive state implementation for managing the given block.
// as this is an example, state is stored in dynamically allocated memory (heap)
template<std::size_t block_size, std::size_t levels>
constexpr
auto
make_buddy(block blk)
{
    using blk_addr_type = buddy_namespace::buddy_block_address<>;
    using layout_type = buddy_namespace::buddy_standard_layout<blk_addr_type, block_size, levels>;
    using traits_type = buddy_namespace::buddy_intrusive_state_traits<layout_type>;
    using state_type = buddy_namespace::buddy_intrusive_state<layout_type, block>;

    layout_type layout{blk};
    auto state_size = traits_type::get_state_size(layout);
    auto state_buffer_ptr = new std::byte[state_size];
    raii_block state_block{block{.base = state_buffer_ptr, .size = state_size}, [](block blk){ delete [] blk.base; }};

    auto state = traits_type::make_state(std::move(layout), std::move(state_block));
    return buddy{std::move(state)};
}

// ====================================================================
// Tests
// ====================================================================

void test_basic_allocation_deallocation()
{
    TEST("allocate / deallocate via buddy");
    // Create a buddy allocator on the buffer
    auto alloc = make_buddy<64, 5>(block{buffer, sizeof(buffer)});
    any_block_allocator_ref ref = alloc;

    // Allocate a 128-byte block
    block b = ref.allocate(128);
    CHECK(b, "allocation returned empty block");
    CHECK(b.size >= 128, "allocated block size is too small");
    CHECK(ref.owns(b), "owns(block) failed");
    CHECK(ref.owns(b.base), "owns(pointer) failed");

    // Deallocate it
    ref.deallocate(b);
    // Allocating again should succeed (same size)
    block b2 = ref.allocate(128);
    CHECK(b2, "re-allocation after deallocate failed");
    // Might be the same address, but no guarantee; just check non‑empty.
    ref.deallocate(b2);
    OK();
}

void test_deallocate_all()
{
    TEST("deallocate_all");
    // Create a buddy allocator on the buffer
    auto alloc = make_buddy<64, 10>(block{buffer, 4096});
    any_block_allocator_ref ref = alloc;

    // Allocate several blocks
    block a = ref.allocate(64);
    block b = ref.allocate(64);
    CHECK(a && b, "initial allocations failed");

    ref.deallocate_all();

    // After deallocate_all, we should be able to allocate up to the maximum again.
    block c = ref.allocate(4096); // large block that would need contiguous space
    CHECK(c, "large allocation after deallocate_all failed");
    ref.deallocate(c);
    OK();
}

void test_owns_semantics()
{
    TEST("owns checks");
    // Create a buddy allocator on the buffer
    auto alloc = make_buddy<64, 5>(block{buffer, 64});
    any_block_allocator_ref ref = alloc;

    // A random stack pointer must not be owned
    std::byte stack_var;
    CHECK(!ref.owns(&stack_var), "should not own stack variable");

    block b = ref.allocate(64);
    CHECK(b, "allocation failed");
    CHECK(ref.owns(b), "owns block should be true");
    CHECK(ref.owns(b.base), "owns pointer should be true");

    // Alter the end pointer to make it out of bounds -> should not own
    block fake = b;
    fake.size += 1;
    CHECK(!ref.owns(fake), "oversized block should not be owned");

    ref.deallocate(b);
    OK();
}

void test_wrapper_copy()
{
    TEST("copy of wrapper");
    auto alloc = make_buddy<64, 5>(block{buffer, sizeof(buffer)});
    any_block_allocator_ref ref1 = alloc;
    any_block_allocator_ref ref2 = ref1;   // copy

    // Both should work and point to the same allocator
    block b1 = ref1.allocate(128);
    CHECK(b1, "first wrapper allocation failed");
    CHECK(ref2.owns(b1), "second wrapper does not own block from first");
    ref2.deallocate(b1);   // deallocate via second wrapper
    block b2 = ref1.allocate(128); // should succeed now
    CHECK(b2, "re-allocation after deallocate via other wrapper failed");
    ref1.deallocate(b2);
    OK();
}

// void test_type_erased_dispatch()
// {
//     TEST("type-erased dispatch (different allocators)");
//     // Use a Buddy and a Pool, both wrapped, to ensure the correct vtable is used.
//     buddy<64, 4> alloc_buddy(block{buffer, sizeof(buffer)/2});       // half of buffer
//     pool<128> alloc_pool(block{buffer + sizeof(buffer)/2, sizeof(buffer)/2});

//     any_block_allocator_ref ref_buddy = alloc_buddy;
//     any_block_allocator_ref ref_pool = alloc_pool;

//     auto b1 = ref_buddy.allocate(200);
//     auto b2 = ref_pool.allocate(200);   // Pool<128> max allocation is 128, so 200 should fail -> empty
//     CHECK(b1, "buddy allocation failed");
//     CHECK(!b2, "pool allocation for 200 should have failed (block size 128)");

//     // Check that owns reporting matches the original allocator
//     CHECK(ref_buddy.owns(b1), "buddy should own its block");
//     CHECK(!ref_pool.owns(b1), "pool should not own buddy's block");

//     ref_buddy.deallocate(b1);
//     OK();
// }

void test_vtable_sharing()
{
    TEST("vtable pointer identity");
    // Two wrappers of the same type should have the same vtable pointer.
    auto b1 = make_buddy<64, 5>(block{buffer, sizeof(buffer)/2});
    auto b2 = make_buddy<64, 5>(block{buffer + sizeof(buffer)/2, sizeof(buffer)/2});
    
    any_block_allocator_ref r1 = b1;
    any_block_allocator_ref r2 = b2;
    CHECK(r1.m_vptr == r2.m_vptr, "vtable pointers differ for same concrete type");


    // Different type -> different vtable pointer
    auto b3 = make_buddy<128, 3>(block{buffer, sizeof(buffer)});

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
        block b = a.allocate(8);
        if (b) a.deallocate(b);
    };

    auto alloc = make_buddy<64, 5>(block{buffer, sizeof(buffer)});
    any_block_allocator_ref ref = alloc;
    use_alloc(ref);   // must compile and run
    OK();
}

void test_move_semantics_in_wrapper()
{
    TEST("move of wrapper");
    auto alloc = make_buddy<64, 5>(block{buffer, sizeof(buffer)});
    any_block_allocator_ref ref = alloc;
    auto ref2 = std::move(ref);
    // After move, ref2 should still work (ref is still valid, pointing to same allocator because it's just a value copy)
    block b = ref2.allocate(64);
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
    // test_type_erased_dispatch();
    test_vtable_sharing();
    test_concept_satisfaction();
    test_move_semantics_in_wrapper();

    std::printf("\n%u tests passed, %u tests failed.\n", passed, failed);
    return (failed > 0) ? 1 : 0;
}
