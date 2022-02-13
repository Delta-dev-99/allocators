#include <allocators/composite/stats.hpp>
#include <allocators/composite/ref.hpp>
#include <allocators/composite/bucketizer.hpp>
#include <allocators/composite/fallback.hpp>
#include <allocators/composite/segregator.hpp>
#include <allocators/composite/timing.hpp>
#include <allocators/basic/slicing.hpp>
#include <allocators/basic/pool.hpp>
#include <allocators/basic/stack.hpp>
#include <allocators/basic/bitmap.hpp>
#include <allocators/basic/buddy.hpp>

#include <allocators/borrowing/bitmap.hpp>
#include <allocators/borrowing/buddy.hpp>



#include <iostream>
#include <vector>
#include <random>
#include <chrono>



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

    // Slicing allocator over 1024b memory block located on the stack
    dd99::memory::Self_Contained_Block<1024> memory_block;
    auto slicing = allocs::Slicing(memory_block);
    
    // Two more allocators that relay allocation to slicing
    auto chop_with_stats = allocs::composite::Stats(allocs::composite::Ref(slicing));
    auto chop_with_stats2 = allocs::composite::Stats(allocs::composite::Ref(slicing));

    // auto chop_with_stats = allocs::composite::Stats(allocs::Slicing(memory_block));

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

    // constexpr auto x = allocs::Buddy<16, 11>::calculate_block_count(1024 << 4);
    // auto y = allocs::Buddy<16, 11>::calculate_block_count(100 * 1024);

    using Buddy_t = allocs::Buddy<16, 2>;
    
    constexpr std::size_t mem_size = 100;
    constexpr auto n = Buddy_t::calculate_block_count(mem_size);
    constexpr auto bits = Buddy_t::bitmap_bits(n);
    constexpr auto bmp = Buddy_t::BMP::calculate_block_count(n) * Buddy_t::BMP::Block_Size;
    constexpr auto unused = mem_size - bmp - n * Buddy_t::Block_Size;
    constexpr auto ratio = Buddy_t::ratio(n);

    for (int i = 0; i < 100; i++)
        std::cout << i << "\t" << Buddy_t::ratio(i) << "\n";
}

void test4()
{
    dd99::memory::Self_Contained_Block<1024> mem;
    dd99::memory::block_allocator::Buddy<32, 3> alloc(mem);

    auto x1 = alloc.allocate(5);
    auto x2 = alloc.allocate(32);
    auto x3 = alloc.allocate(33);
    auto x4 = alloc.allocate(128);
    auto x5 = alloc.allocate(129);
}

void test5()
{
    namespace mem = dd99::memory;
    namespace alloc = mem::block_allocator;

    using allocator_type = alloc::borrowing::Buddy<64, 3>;
    using aux_allocator_type = alloc::Stack;

    const std::size_t mem_size = 128;
    const auto aux_mem_size = allocator_type::calculate_aux_allocation(mem_size);


    mem::Self_Contained_Block<mem_size> memory;
    mem::Self_Contained_Block<aux_mem_size> aux_memory;
    aux_allocator_type aux_allocator(aux_memory);
    allocator_type allocator(memory, aux_allocator);

    auto x1 = allocator.allocate(1);
    auto x2 = allocator.allocate(1);
    auto x3 = allocator.allocate(1);
    allocator.deallocate(x3);
    allocator.deallocate(x2);
    x2 = allocator.allocate(1000);
    allocator.deallocate(x2);
    x2 = allocator.allocate(30);
}

void test6()
{
    // Setup allocators

    namespace mem = dd99::memory;
    namespace alloc = mem::block_allocator;

    using allocator_type = alloc::borrowing::Buddy<64, 5>;
    using aux_allocator_type = alloc::Stack;

    const std::size_t mem_size = 1024 * 1024; // 1Mb
    const auto aux_mem_size = allocator_type::calculate_aux_allocation(mem_size);


    mem::Self_Contained_Block<mem_size> memory;
    mem::Self_Contained_Block<aux_mem_size> aux_memory;
    aux_allocator_type aux_allocator(aux_memory);
    allocator_type allocator(memory, aux_allocator);


    // Setup random number generators

    std::random_device rdev;
    std::mt19937_64 rng_eng(rdev());
    std::discrete_distribution<> boolean_dist({40, 60});
    std::uniform_int_distribution<> collection_index_dist(0, 2);
    std::uniform_int_distribution<> alloc_size_dist(1, 300);
    auto bool_gen = [&] () { return boolean_dist(rng_eng); };
    auto collection_index_gen = [&] () { return collection_index_dist(rng_eng); };
    auto alloc_size_gen = [&] () { return alloc_size_dist(rng_eng); };


    // Setup memory block collections

    std::vector<mem::Block> block_collections[3];
    for (auto & v : block_collections)
        v.reserve(10000);


    // Prepare chronos
    const auto start_time = std::chrono::steady_clock::now();


    // Do the test

    const auto N = 100000000;
    for (int i = 0; i < N; i++)
    {
        const auto collection_index = collection_index_gen();
        auto selected_collection = block_collections[collection_index];
        const bool allocating = bool_gen();

        if (allocating)
        {
            const auto alloc_size = alloc_size_gen();
            selected_collection.push_back(allocator.allocate(alloc_size));
        }
        else
        {
            if (selected_collection.size() == 0) continue;

            const auto block_index = rng_eng() % selected_collection.size();
            allocator.deallocate(selected_collection[block_index]);
            selected_collection.erase(selected_collection.begin() + block_index);
        }
    }

    const auto end_time = std::chrono::steady_clock::now();
    const auto total_duration = std::chrono::duration_cast<std::chrono::duration<long double>>(end_time - start_time);
    const auto iteration_duration = total_duration / N;
    std::cout << "total time: " << total_duration.count() << "\n";
    std::cout << "iteration time: " << iteration_duration.count() << "\n";
}

int main()
{
    // test1();
    // test2();   
    // test4();
    // test5();
    test6();
}