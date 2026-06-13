
#include <allocators/pointer_allocators/basic.hpp>
#include <allocators/pointer_allocators/checked.hpp>
#include <allocators/block_allocators/metrics/stats.hpp>
#include <allocators/block_allocators/composite/fallback.hpp>
#include <allocators/block_allocators/basic/slicing.hpp>
#include <allocators/structures/blocks/self_contained_block.hpp>


void test1()
{
    namespace block_alloc = dd99::memory::block_allocator;
    namespace ptr_alloc = dd99::memory::pointer_allocator;

    dd99::memory::self_contained_block<256> my_memory_ac;
    auto my_memory = my_memory_ac.get_block();
    dd99::memory::self_contained_block<1024> my_memory2_ac;
    auto my_memory2 = my_memory2_ac.get_block();

    

    // Following lines should be noop.
    // Test of correctness of composition definitions
    block_alloc::composite::Fallback(
        block_alloc::Slicing(my_memory),
        block_alloc::Slicing(my_memory));
    block_alloc::composite::Fallback(
        block_alloc::Slicing(my_memory),
        block_alloc::Slicing(my_memory),
        block_alloc::Slicing(my_memory));
    block_alloc::composite::Fallback<block_alloc::Slicing, block_alloc::Slicing>(my_memory, my_memory);
    block_alloc::composite::Fallback<block_alloc::Slicing, block_alloc::Slicing, block_alloc::Slicing>(my_memory, my_memory, my_memory);
    

    
    auto my_ptr_alloc =
        ptr_alloc::Pointer_Checked(
            block_alloc::metrics::Stats(
                block_alloc::composite::Fallback(
                    block_alloc::metrics::Stats(
                        block_alloc::Slicing(my_memory)),
                    block_alloc::metrics::Stats(
                        block_alloc::Slicing(my_memory2)))));

    auto x1 = my_ptr_alloc.allocate(100);
    auto x2 = my_ptr_alloc.allocate(100);
    auto x3 = my_ptr_alloc.allocate(100);
    auto x4 = my_ptr_alloc.allocate(80);
    auto x5 = my_ptr_alloc.allocate(50);
    auto x6 = my_ptr_alloc.allocate(20);
    auto x7 = my_ptr_alloc.allocate(5);

    my_ptr_alloc.deallocate(x5);
    my_ptr_alloc.deallocate(x3);
    my_ptr_alloc.deallocate(x1);
    my_ptr_alloc.deallocate(x6);
    my_ptr_alloc.deallocate(x7);
    my_ptr_alloc.deallocate(x4);
    my_ptr_alloc.deallocate(x2);
}

int main()
{
    test1();
}
