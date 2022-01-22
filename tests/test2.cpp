#include <allocators/composite/stats.hpp>
#include <allocators/composite/ref.hpp>
#include <allocators/composite/bucketizer.hpp>
#include <allocators/composite/fallback.hpp>
#include <allocators/composite/segregator.hpp>
#include <allocators/composite/timing.hpp>
#include <allocators/basic/chop.hpp>
#include <allocators/basic/pool.hpp>
#include <allocators/basic/stack.hpp>
#include <allocators/basic/bitmap.hpp>
#include <allocators/basic/buddy.hpp>

#include <iostream>



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

    dd99::memory::Self_Contained_Block<1024> my_memory;

    allocs::Bitmap<256> bmp_alloc(my_memory);

    auto x1 = bmp_alloc.allocate(100);
    auto x2 = bmp_alloc.allocate(200);
    auto x3 = bmp_alloc.allocate(300);
    auto x4 = bmp_alloc.allocate(100);
    auto x5 = bmp_alloc.allocate(100);

    bmp_alloc.deallocate(x2);

    auto x6 = bmp_alloc.allocate(10);
    bmp_alloc.deallocate_all();
}

void test3()
{
    namespace allocs = dd99::memory::block_allocator;

    dd99::memory::Self_Contained_Block<1024> my_memory;

    allocs::Buddy<16, 3> buddy_alloc(my_memory);

    // constexpr auto x = allocs::Buddy<16, 11>::block_count(1024 << 4);
    // auto y = allocs::Buddy<16, 11>::block_count(100 * 1024);

    using Buddy_t = allocs::Buddy<16, 2>;
    
    constexpr std::size_t mem_size = 100;
    constexpr auto n = Buddy_t::block_count(mem_size);
    constexpr auto bits = Buddy_t::bitmap_bits(n);
    constexpr auto bmp = Buddy_t::BMP::size(n) * Buddy_t::BMP::Block_Size;
    constexpr auto unused = mem_size - bmp - n * Buddy_t::Block_Size;
    constexpr auto ratio = Buddy_t::ratio(n);

    for (int i = 0; i < 100; i++)
        std::cout << i << "\t" << Buddy_t::ratio(i) << "\n";
}

int main()
{
    // test1();
    // test2();   
    test3();
}