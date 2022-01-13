
#include <allocators/pool.hpp>
#include <allocators/chop.hpp>

void pool_test_1()
{
    dd99::memory::Self_Contained_Block<1024> memory_block_1;
    dd99::memory::block_allocator::Pool<512> pool(memory_block_1);

    auto x1 = pool.allocate(1);
    auto x2 = pool.allocate(1000);
    auto x3 = pool.allocate(1);
    auto x4 = pool.allocate(1);

    pool.deallocate(x2);
    auto x5 = pool.allocate(1);
    pool.deallocate(x3);
    auto x6 = pool.allocate(1);

    pool.deallocate_all();
    auto x7 = pool.allocate(1);
    auto x8 = pool.allocate(1);
    auto x9 = pool.allocate(1);
    auto x10 = pool.allocate(1);
}

void chop_test_1()
{
    dd99::memory::Self_Contained_Block<1024> memory_block_1;
    dd99::memory::block_allocator::Chop chop(memory_block_1);

    auto x1 = chop.allocate(1);
    auto x2 = chop.allocate(100);
    auto x3 = chop.allocate(500);
    auto x4 = chop.allocate(400);
    auto x5 = chop.allocate(100);
    auto x6 = chop.allocate(100);
    chop.deallocate(x2);
    auto x7 = chop.allocate(50);
    auto x8 = chop.allocate(50);
    auto x9 = chop.allocate(50);
    auto x10 = chop.allocate(1);
    auto x11 = chop.allocate(1);
    auto x12 = chop.allocate(1);
    auto x13 = chop.allocate(1);
    auto x14 = chop.allocate(1);
}

void chop_test_2()
{
    dd99::memory::Self_Contained_Block<1024> memory_block_1;
    dd99::memory::block_allocator::Chop chop(memory_block_1);

    auto x1 = chop.allocate(1);
    auto x2 = chop.allocate(30);
    auto x3 = chop.allocate(50);
    auto x4 = chop.allocate(40);
    auto x5 = chop.allocate(80);
    auto x6 = chop.allocate(30);
    chop.deallocate(x2);
    auto x7 = chop.allocate(50);
    auto x8 = chop.allocate(50);
    auto x9 = chop.allocate(20);
    auto x10 = chop.allocate(80);
    auto x11 = chop.allocate(80);
    auto x12 = chop.allocate(40);
    auto x13 = chop.allocate(70);
    auto x14 = chop.allocate(80);

    chop.deallocate(x1);
    // chop.deallocate(x2); // Already deallocated
    chop.deallocate(x3);
    chop.deallocate(x5);
    chop.deallocate(x6);
    chop.deallocate(x4);
    chop.deallocate(x10);
    chop.deallocate(x8);

    auto x15 = chop.allocate(50);
    auto x16 = chop.allocate(20);
    x1 = chop.allocate(300);
    x2 = chop.allocate(100);
    x3 = chop.allocate(120);
    x4 = chop.allocate(120);
    x5 = chop.allocate(120);
    x6 = chop.allocate(30);
}

int main()
{
    pool_test_1();
    chop_test_1();
    chop_test_2();
}
