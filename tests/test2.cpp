#include <allocators/composite/stats.hpp>
#include <allocators/composite/ref.hpp>
#include <allocators/composite/bucketizer.hpp>
#include <allocators/composite/fallback.hpp>
#include <allocators/composite/segregator.hpp>
#include <allocators/composite/timing.hpp>
#include <allocators/chop.hpp>
#include <allocators/pool.hpp>
#include <allocators/stack.hpp>

void do_something_with_allocator(dd99::memory::block_allocator::Allocator& alloc)
{
    auto x1 = alloc.allocate(10);
    auto x2 = alloc.allocate(10);
    alloc.deallocate(x1);
    alloc.deallocate(x2);
}

void test1()
{
    namespace allocs = dd99::memory::block_allocator;

    // Chop allocator over 1024b memory block located on the stack
    dd99::memory::Self_Contained_Block<1024> memory_block;
    auto chop = allocs::Chop(memory_block);
    
    // Two more allocators that relay allocation to chop
    auto chop_with_stats = allocs::composite::Stats(allocs::composite::Ref(chop));
    auto chop_with_stats2 = allocs::composite::Stats(allocs::composite::Ref(chop));

    // auto chop_with_stats = allocs::composite::Stats(allocs::Chop(memory_block));

    auto x1 = chop_with_stats.allocate(1);
    auto x2 = chop_with_stats.allocate(30);
    auto x3 = chop_with_stats.allocate(50);
    auto x4 = chop_with_stats.allocate(40);
    auto x5 = chop_with_stats.allocate(80);
    auto x6 = chop_with_stats.allocate(30);
    chop_with_stats.deallocate(x2);
    auto x7 = chop_with_stats.allocate(50);
    auto x8 = chop_with_stats.allocate(50);
    auto x9 = chop_with_stats.allocate(20);
    auto x10 = chop_with_stats.allocate(80);
    auto x11 = chop_with_stats.allocate(80);
    auto x12 = chop_with_stats.allocate(40);
    auto x13 = chop_with_stats.allocate(70);
    auto x14 = chop_with_stats.allocate(80);

    chop_with_stats.deallocate(x1);
    // chop_with_stats.deallocate(x2); // Already deallocated
    chop_with_stats.deallocate(x3);
    chop_with_stats.deallocate(x5);
    chop_with_stats.deallocate(x6);
    chop_with_stats.deallocate(x4);
    chop_with_stats.deallocate(x10);
    chop_with_stats.deallocate(x8);

    auto x15 = chop_with_stats.allocate(50);
    auto x16 = chop_with_stats.allocate(20);
    x1 = chop_with_stats.allocate(300);
    x2 = chop_with_stats.allocate(100);
    x3 = chop_with_stats.allocate(120);
    x4 = chop_with_stats.allocate(120);
    x5 = chop_with_stats.allocate(120);
    x6 = chop_with_stats.allocate(30);

    do_something_with_allocator(chop_with_stats2);
}

void test2()
{
    namespace allocs = dd99::memory::block_allocator;
}

int main()
{
    // test1();
    test2();   
}