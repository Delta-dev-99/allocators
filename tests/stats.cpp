#include <allocators/metrics/stats.hpp>
#include <allocators/metrics/timing.hpp>
#include <allocators/composite/ref.hpp>
#include <allocators/composite/quantizer.hpp>
#include <allocators/composite/fallback.hpp>
#include <allocators/composite/segregator.hpp>
#include <allocators/basic/slicing.hpp>
#include <allocators/basic/pool.hpp>
#include <allocators/basic/stack.hpp>
#include <allocators/basic/bitmap.hpp>
#include <allocators/basic/buddy.hpp>

#include <allocators/borrowing/bitmap.hpp>
#include <allocators/borrowing/buddy.hpp>

#include <allocators/acquire_memory/self_contained_block.hpp>



#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>


namespace mem = dd99::memory;
namespace alloc = mem::block_allocator;



auto get_repeatable_rng_engine()
{
    static std::random_device rdev;
    static auto seed = rdev();

    return std::mt19937_64(seed);
}

template <class Distribution>
void stat_allocator(auto & allocator, std::size_t iterations, Distribution allocation_size_distribution, double allocation_probability = 0.65)
{
    auto allocator_with_stats = alloc::metrics::Stats(alloc::metrics::Timing(alloc::composite::Ref(allocator)));
    using alloc_type = decltype(allocator_with_stats);

    std::vector<mem::Block> memory_blocks;

    // auto rng_eng = get_repeatable_rng_engine();
    std::mt19937_64 rng_eng(std::random_device{}());
    std::bernoulli_distribution boolean_dist(allocation_probability);

    for (std::size_t i = 0; i < iterations; ++i)
    {
        const bool allocate = memory_blocks.empty() || boolean_dist(rng_eng);

        if (allocate)
        {
            const auto allocation_size = allocation_size_distribution(rng_eng);
            const auto memory_block = allocator_with_stats.allocate(allocation_size);
            memory_blocks.push_back(memory_block);
        }
        else
        {
            const auto block_index = std::uniform_int_distribution<>(0, memory_blocks.size() - 1)(rng_eng);
            auto memory_block = memory_blocks[block_index];
            memory_blocks.erase(memory_blocks.begin() + block_index);
            allocator_with_stats.deallocate(memory_block);
        }
    }

    // total allocation duration / number of allocations
    const auto mean_allocation_duration =
        std::chrono::duration_cast<std::chrono::duration<long double, std::nano>>(
            allocator_with_stats.get_timing_data().total_duration[alloc_type::Timed_Operation::Successful_Allocation])
        / allocator_with_stats.get_stats().total[alloc_type::Stats_Data::Allocation];

    // total deallocation duration / number of deallocations
    const auto mean_deallocation_duration =
        std::chrono::duration_cast<std::chrono::duration<long double, std::nano>>(
            allocator_with_stats.get_timing_data().total_duration[alloc_type::Timed_Operation::Successful_Deallocation])
        / allocator_with_stats.get_stats().total[alloc_type::Stats_Data::Deallocation];

    const auto currently_allocated_size =
        allocator_with_stats.get_stats().total[alloc_type::Stats_Data::Allocated_Size]
        - allocator_with_stats.get_stats().total[alloc_type::Stats_Data::Deallocated_Size];

    std::cout << std::setw(10) << iterations << "    "
              << std::setw(15) << mean_allocation_duration.count()
              << std::setw(15) << mean_deallocation_duration.count() << "    "
              << std::setw(9) << allocator_with_stats.get_stats().total[alloc_type::Stats_Data::Allocation] << "  "
              << std::setw(9) << allocator_with_stats.get_stats().total[alloc_type::Stats_Data::Failed_Allocation] << "    "
              << std::setw(10) << currently_allocated_size
              << "\n";
}

int main()
{
    std::cout.precision(4);
    std::cout << std::fixed;
    std::cout << std::setw(10) << "iterations" << "    "
              << std::setw(30) << "mean times (alloc, dealloc) ns" << "    "
              << std::setw(20) << "allocations, failed" << "    "
              << std::setw(10) << "allocated"
              << "\n";

    using aux_allocator_type = alloc::Stack;
    using allocator_type = alloc::borrowing::Buddy<64, 11, aux_allocator_type>;

    constexpr std::size_t mem_size = 1 << 20;
    constexpr auto aux_mem_size = allocator_type::calculate_aux_mem_size(mem_size);

    mem::Self_Contained_Block<mem_size> memory;
    mem::Self_Contained_Block<aux_mem_size> aux_memory;

    aux_allocator_type aux_allocator(aux_memory);
    allocator_type allocator(memory, std::move(aux_allocator));

    for (int i = 0; i < 200; i++)
    {
        const auto offset = 10 + i;
        const auto base = 1.15;
        const auto factor = 0.2 * (i + 1);
        const auto power = 0.2 * (i + 1);
        const std::size_t iterations = offset + factor * std::pow(base, power);

        auto allocation_size_distribution = std::uniform_int_distribution<>(1, 800);
        stat_allocator(allocator, iterations, std::move(allocation_size_distribution));
        allocator.deallocate_all();
    }
}
