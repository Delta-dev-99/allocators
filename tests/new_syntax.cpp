
#include <allocators/structures/blocks/self_contained_block.hpp>
#include <allocators/block_allocators/new.hpp>
#include <allocators/block_allocators/basic/stack/stack.hpp>
#include <allocators/pointer_allocators/basic.hpp>
#include <iostream>



class A
{
public:
    A() { std::cout << "A constructed\n"; }
    ~A() { std::cout << "A destroyed\n"; }
};


int main()
{
    // TODO: enable

    namespace mem = dd99_allocators_namespace;
    namespace blk_alloc = mem::block_allocator;
    namespace ptr_alloc = mem::pointer_allocator;

    mem::self_contained_block<512> stack_block, stack_ptr_block;
    auto my_block_allocator = blk_alloc::Stack{stack_block.get_block()};
    auto my_ptr_allocator = ptr_alloc::Basic<decltype(my_block_allocator)>{stack_ptr_block.get_block()};


    {
        auto my_allocated_A           = mem::allocator_new<A>(my_block_allocator);
        auto my_allocated_A_array     = mem::allocator_new<A[5]>(my_block_allocator);
        auto my_allocated_A_dyn_array = mem::allocator_new<A[]>(my_block_allocator, 5);



        mem::allocator_delete(my_block_allocator, my_allocated_A_dyn_array);
        mem::allocator_delete(my_block_allocator, my_allocated_A_array);
        mem::allocator_delete(my_block_allocator, my_allocated_A);
    }


    {
        // auto my_allocated_A           = mem::allocator_new<A>(my_ptr_allocator);
        // auto my_allocated_A_array     = mem::allocator_new<A[5]>(my_ptr_allocator);
        // auto my_allocated_A_dyn_array = mem::allocator_new<A[]>(my_ptr_allocator, 5);


        // mem::allocator_delete(my_ptr_allocator, my_allocated_A_dyn_array);
        // mem::allocator_delete(my_ptr_allocator, my_allocated_A_array);
        // mem::allocator_delete(my_ptr_allocator, my_allocated_A);
    }

    {
        auto A_ptr = my_ptr_allocator.allocate(sizeof(A), alignof(A));
        auto my_allocated_A = ::new(A_ptr) A{};

        auto A_array_ptr = my_ptr_allocator.allocate(sizeof(A[5]), alignof(A[5]));
        auto my_allocated_A_array = ::new(A_array_ptr) A[5]{};

        int count; count = 5;
        auto A_dyn_array_ptr = my_ptr_allocator.allocate(sizeof(A)*count, alignof(A));
        auto my_allocated_A_dyn_array = ::new(A_dyn_array_ptr) A[count]{};

        

        std::destroy_n(my_allocated_A_dyn_array, count);
        my_ptr_allocator.deallocate(A_dyn_array_ptr);

        std::destroy_n(my_allocated_A_array, 5);
        my_ptr_allocator.deallocate(A_array_ptr);

        my_allocated_A->~A();
        my_ptr_allocator.deallocate(A_ptr);
    }
}
