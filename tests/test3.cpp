
#include <allocators/basic/chop.hpp>
#include <allocators/composite/fallback.hpp>
#include <allocators/composite/stats.hpp>
#include <allocators/pointer/basic.hpp>
#include <allocators/pointer/checked.hpp>


void test1()
{
    namespace block_alloc = dd99::memory::block_allocator;
    namespace ptr_alloc = dd99::memory::pointer_allocator;

    dd99::memory::Self_Contained_Block<256> my_memory;
    dd99::memory::Self_Contained_Block<1024> my_memory2;

    

    // Following lines should be noop.
    // Test of correctness of composition definitions
    block_alloc::composite::Fallback_Allocator(
        block_alloc::Chop(my_memory),
        block_alloc::Chop(my_memory));
    block_alloc::composite::Fallback_Allocator(
        block_alloc::Chop(my_memory),
        block_alloc::Chop(my_memory),
        block_alloc::Chop(my_memory));
    block_alloc::composite::Fallback_Allocator<block_alloc::Chop, block_alloc::Chop>(my_memory, my_memory);
    block_alloc::composite::Fallback_Allocator<block_alloc::Chop, block_alloc::Chop, block_alloc::Chop>(my_memory, my_memory, my_memory);
    

    
    auto my_ptr_alloc =
        ptr_alloc::Pointer_Checked(
            block_alloc::composite::Stats(
                block_alloc::composite::Fallback_Allocator(
                    block_alloc::composite::Stats(
                        block_alloc::Chop(my_memory)),
                    block_alloc::composite::Stats(
                        block_alloc::Chop(my_memory2)))));

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
