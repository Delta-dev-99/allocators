
#include <allocators/acquire_memory/self_contained_block.hpp>
#include <allocators/new.hpp>
#include <allocators/basic/stack.hpp>
#include <allocators/pointer/new.hpp>
#include <allocators/pointer/basic.hpp>



int main()
{
    namespace mem = dd99::memory;
    namespace blk_alloc = mem::block_allocator;
    namespace ptr_alloc = mem::pointer_allocator;

    mem::Self_Contained_Block<512> stack_block, stack_ptr_block;
    auto my_block_allocator = blk_alloc::Stack(stack_block);
    auto my_ptr_allocator = ptr_alloc::Basic<blk_alloc::Stack>(stack_ptr_block);



    auto my_allocated_int           = mem::allocator_new<int>(my_block_allocator);
    auto my_allocated_int_array     = mem::allocator_new<int[5]>(my_block_allocator);
    auto my_allocated_int_dyn_array = mem::allocator_new<int[]>(my_block_allocator, 5);



    mem::allocator_delete(my_block_allocator, my_allocated_int_dyn_array);
    mem::allocator_delete(my_block_allocator, my_allocated_int_array);
    mem::allocator_delete(my_block_allocator, my_allocated_int);
}
