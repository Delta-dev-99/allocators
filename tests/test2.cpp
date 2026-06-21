#include <allocators/allocators.hpp>



#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>



// void do_something_with_allocator(dd99::memory::block_allocator::Allocator& alloc)
// {
//     auto x1 = alloc.allocate(10);
//     auto x2 = alloc.allocate(10);
//     alloc.deallocate(x1);
//     alloc.deallocate(x2);
// }

// void test1()
// {
//     namespace allocs = dd99::memory::block_allocator;

//     // Slicing allocator over 1024b memory block located on the stack
//     dd99::memory::self_contained_block<1024> memory_block;
//     auto slicing = allocs::Slicing(memory_block);
    
//     // Two more allocators that relay allocation to slicing
//     auto chop_with_stats = allocs::metrics::Stats(allocs::composite::Ref(slicing));
//     auto chop_with_stats2 = allocs::metrics::Stats(allocs::composite::Ref(slicing));

//     // auto chop_with_stats = allocs::composite::Stats(allocs::Slicing(memory_block));

//     auto x1 = chop_with_stats.allocate(1);
//     auto x2 = chop_with_stats.allocate(30);
//     auto x3 = chop_with_stats.allocate(50);
//     auto x4 = chop_with_stats.allocate(40);
//     auto x5 = chop_with_stats.allocate(80);
//     auto x6 = chop_with_stats.allocate(30);
//     chop_with_stats.deallocate(x2);
//     auto x7 = chop_with_stats.allocate(50);
//     auto x8 = chop_with_stats.allocate(50);
//     auto x9 = chop_with_stats.allocate(20);
//     auto x10 = chop_with_stats.allocate(80);
//     auto x11 = chop_with_stats.allocate(80);
//     auto x12 = chop_with_stats.allocate(40);
//     auto x13 = chop_with_stats.allocate(70);
//     auto x14 = chop_with_stats.allocate(80);

//     chop_with_stats.deallocate(x1);
//     // chop_with_stats.deallocate(x2); // Already deallocated
//     chop_with_stats.deallocate(x3);
//     chop_with_stats.deallocate(x5);
//     chop_with_stats.deallocate(x6);
//     chop_with_stats.deallocate(x4);
//     chop_with_stats.deallocate(x10);
//     chop_with_stats.deallocate(x8);

//     auto x15 = chop_with_stats.allocate(50);
//     auto x16 = chop_with_stats.allocate(20);
//     x1 = chop_with_stats.allocate(300);
//     x2 = chop_with_stats.allocate(100);
//     x3 = chop_with_stats.allocate(120);
//     x4 = chop_with_stats.allocate(120);
//     x5 = chop_with_stats.allocate(120);
//     x6 = chop_with_stats.allocate(30);

//     do_something_with_allocator(chop_with_stats2);
// }

// void test2()
// {
//     namespace allocs = dd99::memory::block_allocator;

//     dd99::memory::self_contained_block<1024> my_memory;

//     allocs::Bitmap<256> bmp_alloc(my_memory);

//     auto x1 = bmp_alloc.allocate(100);
//     auto x2 = bmp_alloc.allocate(200);
//     auto x3 = bmp_alloc.allocate(300);
//     auto x4 = bmp_alloc.allocate(100);
//     auto x5 = bmp_alloc.allocate(100);

//     bmp_alloc.deallocate(x2);

//     auto x6 = bmp_alloc.allocate(10);
//     bmp_alloc.deallocate_all();
// }

// void test3()
// {
//     namespace allocs = dd99::memory::block_allocator;

//     dd99::memory::self_contained_block<1024> my_memory;

//     allocs::Buddy<16, 3> buddy_alloc(my_memory);

//     // constexpr auto x = allocs::Buddy<16, 11>::calculate_block_count(1024 << 4);
//     // auto y = allocs::Buddy<16, 11>::calculate_block_count(100 * 1024);

//     using Buddy_t = allocs::Buddy<16, 2>;
//     using Buddy_Bitmap_t = dd99::memory::structure::Bitmap<>;
    
//     constexpr std::size_t mem_size = 100;
//     constexpr auto block_count = Buddy_t::calculate_basic_block_count(mem_size);
//     constexpr auto bits = Buddy_t::calculate_bmp_bit_count(block_count);
//     constexpr auto bmp = Buddy_Bitmap_t::calculate_block_count(bits) * Buddy_Bitmap_t::Block_Size;
//     constexpr auto unused = mem_size - bmp - block_count * Buddy_t::Block_Size;
//     constexpr auto unused_ratio = double(unused)/mem_size;

//     std::cout << "unused ratio: " << unused_ratio << "\n";
// }

void test4()
{
    // TODO:
    // dd99::memory::self_contained_block<1024> mem;
    // dd99::memory::block_allocator::buddy<32, 3> alloc(mem);

    // auto x1 = alloc.allocate(5);
    // auto x2 = alloc.allocate(32);
    // auto x3 = alloc.allocate(33);
    // auto x4 = alloc.allocate(128);
    // auto x5 = alloc.allocate(129);
}

// void test5()
// {
//     namespace mem = dd99::memory;
//     namespace alloc = mem::block_allocator;

//     using allocator_type = alloc::borrowing::Buddy<64, 3>;
//     using aux_allocator_type = alloc::Stack;

//     const std::size_t mem_size = 128;
//     const auto aux_mem_size = allocator_type::calculate_aux_allocation(mem_size);


    // mem::self_contained_block<mem_size> memory;
    // mem::self_contained_block<aux_mem_size> aux_memory;
    // aux_allocator_type aux_allocator(aux_memory);
    // allocator_type allocator(memory, aux_allocator);

    // auto x1 = allocator.allocate(1);
    // auto x2 = allocator.allocate(1);
    // auto x3 = allocator.allocate(1);
    // allocator.deallocate(x3);
    // allocator.deallocate(x2);
    // x2 = allocator.allocate(1000);
    // allocator.deallocate(x2);
    // x2 = allocator.allocate(30);
// }

// void allocation_timing(dd99::memory::block_allocator::Allocator & allocator)
// {
//     namespace mem = dd99::memory;

//     std::cout.precision(10);
//     std::cout.fill('0');

//     // Setup random number generators

//     std::random_device rdev;
//     std::mt19937_64 rng_eng(rdev());
//     std::discrete_distribution<> boolean_dist({40, 60});
//     std::uniform_int_distribution<> alloc_size_dist(1, 300);
//     auto bool_gen = [&] () { return boolean_dist(rng_eng); };
//     auto alloc_size_gen = [&] () { return alloc_size_dist(rng_eng); };


//     // Do the test

//     const int NN = 50;
//     const int NF = 100;
//     const int NE = 2;
//     const auto NEF = 0.2;
//     for (int k = 0; k < NN; k++)
//     {
//         allocator.deallocate_all();

//         // Setup memory block collections

//         std::vector<mem::block> block_collection;
//         block_collection.reserve(10000);



//         std::chrono::nanoseconds total_duration{0}, total_allocation_duration{0}, total_deallocation_duration{0};
//         int total_allocations{0}, total_deallocations{0}, total_failed_allocations{0}, first_failed_allocation_iteration{-1};

//         const int N = NF * std::pow(NE, k * NEF);
//         for (int i = 0; i < N; i++)
//         {
//             const bool allocating = bool_gen();

//             if (allocating)
//             {
//                 const auto alloc_size = alloc_size_gen();


//                 const auto allocation_start_time = std::chrono::steady_clock::now();
//                 const auto block = allocator.allocate(alloc_size);
//                 const auto allocation_end_time = std::chrono::steady_clock::now();

//                 block_collection.push_back(block);

//                 const auto allocation_duration = allocation_end_time - allocation_start_time;
//                 total_duration += allocation_duration;
//                 total_allocation_duration += allocation_duration;
//                 total_allocations++;
//                 if (!block)
//                 {
//                     if (!total_failed_allocations) first_failed_allocation_iteration = i;
//                     total_failed_allocations++;
//                 }
//             }
//             else
//             {
//                 if (block_collection.size() == 0) continue;
                
//                 const auto block_index = std::uniform_int_distribution<>(0, block_collection.size() - 1)(rng_eng);
//                 const auto block = block_collection[block_index];


//                 const auto deallocation_start_time = std::chrono::steady_clock::now();
//                 allocator.deallocate(block);
//                 const auto deallocation_end_time = std::chrono::steady_clock::now();

//                 block_collection.erase(block_collection.begin() + block_index);

//                 const auto deallocation_duration = deallocation_end_time - deallocation_start_time;
//                 total_duration += deallocation_duration;
//                 total_deallocation_duration += deallocation_duration;
//                 total_deallocations++;
//             }
//         }

//         // const auto end_time = std::chrono::steady_clock::now();
//         // total_duration = std::chrono::duration_cast<std::chrono::duration<long double>>(end_time - start_time);

//         const auto mean_iteration_duration = std::chrono::duration_cast<std::chrono::duration<long double, std::nano>>(total_duration) / N;
//         const auto mean_allocation_duration = std::chrono::duration_cast<std::chrono::duration<long double, std::nano>>(total_allocation_duration) / total_allocations;
//         const auto mean_deallocation_duration = std::chrono::duration_cast<std::chrono::duration<long double, std::nano>>(total_deallocation_duration) / total_deallocations;


//         std::cout << k << ": iterations: " << N << "\t"
//                   << "time(total): " << std::chrono::duration_cast<std::chrono::duration<long double, std::nano>>(total_duration).count() << "\t"
//                   << "allocations: " << total_allocations << "\t"
//                   << "allocations(failed): " << total_failed_allocations << "\t"
//                   << "iteration(first-failed): " << first_failed_allocation_iteration << "\t"
//                   << "time(allocation): " << mean_allocation_duration.count() << "\t"
//                   << "de-allocations: " << total_deallocations << "\t"
//                   << "time(de-allocation): " << mean_deallocation_duration.count() << "\t"
//                   << "iteration time: " << mean_iteration_duration.count() << "\n";
//     }
// }

// void time_allocators()
// {
//     // Setup allocators

//     namespace mem = dd99::memory;
//     namespace alloc = mem::block_allocator;

//     {   // buddy<64,5> allocator
//         std::cout << "Allocator: buddy<64,5>\n";
//         using allocator_type = alloc::borrowing::Buddy<64, 5>;
//         using aux_allocator_type = alloc::degenerate::Null;

//         const std::size_t mem_size = 1024 * 1024; // 1Mb
//         const auto aux_mem_size = allocator_type::calculate_aux_allocation(mem_size);


//         // mem::self_contained_block<mem_size> memory;
//         // mem::self_contained_block<aux_mem_size> aux_memory;
//         // aux_allocator_type aux_allocator(aux_memory);
//         // allocator_type allocator(memory, aux_allocator);

//         // allocation_timing(allocator);
//         // std::cout << "\n\n";
//     }

//     // {   // buddy<64,8> allocator
//     //     std::cout << "Allocator: buddy<64,8>\n";
//     //     using allocator_type = alloc::borrowing::Buddy<64, 8>;
//     //     using aux_allocator_type = alloc::Stack;

//     //     const std::size_t mem_size = 1024 * 1024; // 1Mb
//     //     const auto aux_mem_size = allocator_type::calculate_aux_allocation(mem_size);


//     //     mem::self_contained_block<mem_size> memory;
//     //     mem::self_contained_block<aux_mem_size> aux_memory;
//     //     aux_allocator_type aux_allocator(aux_memory);
//     //     allocator_type allocator(memory, aux_allocator);

//     //     allocation_timing(allocator);
//     //     std::cout << "\n\n";
//     // }

//     // {   // buddy<64,11> allocator
//     //     std::cout << "Allocator: buddy<64,11>\n";
//     //     using allocator_type = alloc::borrowing::Buddy<64, 11>;
//     //     using aux_allocator_type = alloc::Constant;

//     //     const std::size_t mem_size = 1024 * 1024; // 1Mb
//     //     const auto aux_mem_size = allocator_type::calculate_aux_allocation(mem_size);


//     //     mem::self_contained_block<mem_size> memory;
//     //     mem::self_contained_block<aux_mem_size> aux_memory;
//     //     aux_allocator_type aux_allocator(aux_memory);
//     //     allocator_type allocator(memory, aux_allocator);

//     //     allocation_timing(allocator);
//     //     std::cout << "\n\n";
//     // }

//     // {   // buddy<8,11> allocator
//     //     std::cout << "Allocator: buddy<8,11>\n";
//     //     using allocator_type = alloc::borrowing::Buddy<16, 6>;
//     //     using aux_allocator_type = alloc::Stack;

//     //     const std::size_t mem_size = 1024 * 1024; // 1Mb
//     //     const auto aux_mem_size = allocator_type::calculate_aux_allocation(mem_size);


//     //     mem::self_contained_block<mem_size> memory;
//     //     mem::self_contained_block<aux_mem_size> aux_memory;
//     //     aux_allocator_type aux_allocator(aux_memory);
//     //     allocator_type allocator(memory, aux_allocator);

//     //     allocation_timing(allocator);
//     //     std::cout << "\n\n";
//     // }



//     // {   // slicing allocator
//     //     std::cout << "Allocator: slicing\n";
//     //     using allocator_type = alloc::Slicing;

//     //     const std::size_t mem_size = 1024 * 1024; // 1Mb


//     //     mem::self_contained_block<mem_size> memory;
//     //     allocator_type allocator(memory);

//     //     allocation_timing(allocator);
//     //     std::cout << "\n\n";
//     // }

//     // {   // stack allocator
//     //     std::cout << "Allocator: stack\n";
//     //     using allocator_type = alloc::Stack;

//     //     const std::size_t mem_size = 1024 * 1024; // 1Mb


//     //     mem::self_contained_block<mem_size> memory;
//     //     allocator_type allocator(memory);

//     //     allocation_timing(allocator);
//     //     std::cout << "\n\n";
//     // }

//     // {   // pool<300> allocator
//     //     std::cout << "Allocator: pool<300>\n";
//     //     using allocator_type = alloc::Pool<300>;

//     //     const std::size_t mem_size = 1024 * 1024; // 1Mb


//     //     mem::self_contained_block<mem_size> memory;
//     //     allocator_type allocator(memory);

//     //     allocation_timing(allocator);
//     //     std::cout << "\n\n";
//     // }
// }


void instantiation_compilation_test1()
{
    // constexpr auto mem_size = 1024;
    // dd99::memory::self_contained_block<mem_size> my_memory, my_memory2;

    // using aux_alloc_t = dd99::memory::block_allocator::Stack;
    // using alloc_t = dd99::memory::block_allocator::borrowing::Buddy<32, 6, aux_alloc_t>;

    // constexpr auto aux_mem_size = alloc_t::calculate_aux_mem_size(mem_size);
    // dd99::memory::self_contained_block<aux_mem_size> aux_memory, aux_memory2;

    // {
    //     alloc_t my_alloc(my_memory, aux_memory);
    //     // auto copy_constructed = my_alloc;
    //     auto move_constructed = std::move(my_alloc);
    // }

    // {
    //     alloc_t my_alloc(my_memory, aux_memory);
    //     // alloc_t copied_to(my_memory2, aux_memory2);
    //     // copied_to = my_alloc;
    // }

    // {
    //     alloc_t my_alloc(my_memory, aux_memory);
    //     alloc_t moved_to(my_memory2, aux_memory2);
    //     moved_to = std::move(my_alloc);

    //     // auto x = my_alloc.allocate(30);
    //     // auto y = moved_to.allocate(30);
    //     // my_alloc.deallocate(x);
    //     // moved_to.deallocate(y);
    // }
}


void instantiation_compilation_test2()
{
    // constexpr auto mem_size = 256;
    // dd99::memory::self_contained_block<mem_size> my_memory, aux_memory;
    
    // {
    //     using aux_alloc_t = dd99::memory::block_allocator::Stack;
    //     dd99::memory::block_allocator::borrowing::Buddy<32, 12, aux_alloc_t> my_alloc(my_memory, aux_memory);
    // }

    // {
    //     dd99::memory::block_allocator::Stack aux_alloc(aux_memory);
    //     using aux_alloc_t = dd99::memory::block_allocator::composite::Ref<dd99::memory::block_allocator::Stack>;
    //     dd99::memory::block_allocator::borrowing::Buddy<32, 12, aux_alloc_t> my_alloc(my_memory, aux_alloc);
    // }
}


int main()
{
    // test1();
    // test2();   
    test4();
    // test5();
    // time_allocators();

    instantiation_compilation_test1();
    instantiation_compilation_test2();
}