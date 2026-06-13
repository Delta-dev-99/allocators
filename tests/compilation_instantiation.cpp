
#include <allocators/allocators.hpp>
#include <allocators/block_allocators/utility/switch.hpp>
#include <allocators/structures/blocks/self_contained_block.hpp>
#include <iomanip>
#include <iostream>


struct Memory_Request
{
    int priority; // lower number means higher priority. 0 is max.
    std::size_t size;

    std::size_t get_request() { return size; }
};


std::size_t mem_req_switch_func(Memory_Request mem_req)
{
    if (mem_req.size > 1000 * (10 - mem_req.priority))
        return std::size_t(-1);

    return (mem_req.priority / 3) % 3;
}


void print_mem(dd99::memory::block mem)
{
    std::cout << std::setw(20) << std::hex << mem.base << " :   "
              << std::setw(10) << std::dec << mem.size << "\n";
}


int main()
{
    auto memories = new dd99::memory::Self_Contained_Block<1024>[3];
    // dd99::memory::Heap_Block<1024> memories[3];

    dd99::memory::block_allocator::utility::Switch my_switch(mem_req_switch_func,
        dd99::memory::block_allocator::Slicing{memories[0]},
        dd99::memory::block_allocator::Stack{memories[1]},
        dd99::memory::block_allocator::Slicing{memories[2]}
    );

    auto x = my_switch.allocate(Memory_Request{.priority = 0, .size = 1024});
    print_mem(x);

    auto y = my_switch.allocate(Memory_Request{.priority = 0, .size = 1024});
    print_mem(y);

    my_switch.deallocate(x);
    auto z = my_switch.allocate(Memory_Request{.priority = 0, .size = 1024});
    print_mem(z);
    
    my_switch.deallocate(y);
    my_switch.deallocate(z);
}
