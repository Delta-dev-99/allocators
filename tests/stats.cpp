#include <allocators/block_allocators/metrics/stats.hpp>
#include <allocators/block_allocators/metrics/timing.hpp>
#include <allocators/block_allocators/composite/ref.hpp>
#include <allocators/block_allocators/composite/quantizer.hpp>
#include <allocators/block_allocators/composite/fallback.hpp>
#include <allocators/block_allocators/composite/segregator.hpp>
#include <allocators/block_allocators/basic/slicing/slicing.hpp>
#include <allocators/block_allocators/basic/pool/pool.hpp>
#include <allocators/block_allocators/basic/stack/stack.hpp>
#include <allocators/block_allocators/basic/bitmap/bitmap.hpp>
#include <allocators/block_allocators/basic/buddy/buddy.hpp>
#include <allocators/block_allocators/basic/buddy/state_implementations/buddy_intrusive_state.hpp>
#include <allocators/block_allocators/basic/buddy/state_implementations/buddy_fused_state.hpp>

#include <allocators/structures/blocks/self_contained_block.hpp>
#include <allocators/structures/blocks/raii_block.hpp>



#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>


namespace mem = dd99_allocators_namespace;
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
    auto & alloc_timing = allocator_with_stats.m_sub_allocator;
    using alloc_stats_type = decltype(allocator_with_stats);
    using alloc_timing_type = std::remove_reference_t<decltype(allocator_with_stats.m_sub_allocator)>;

    std::vector<mem::block> memory_blocks;

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
            allocator_with_stats.m_sub_allocator.get_timing_data().total_duration[alloc_timing_type::Timed_Operation::Successful_Allocation])
        / allocator_with_stats.get_stats().total[alloc_stats_type::Stats_Data::Allocation];

    // total deallocation duration / number of deallocations
    const auto mean_deallocation_duration =
        std::chrono::duration_cast<std::chrono::duration<long double, std::nano>>(
            allocator_with_stats.m_sub_allocator.get_timing_data().total_duration[alloc_timing_type::Timed_Operation::Successful_Deallocation])
        / allocator_with_stats.get_stats().total[alloc_stats_type::Stats_Data::Deallocation];

    const auto currently_allocated_size =
        allocator_with_stats.get_stats().total[alloc_stats_type::Stats_Data::Allocated_Size]
        - allocator_with_stats.get_stats().total[alloc_stats_type::Stats_Data::Deallocated_Size];

    std::cout << std::setw(10) << iterations << "    "
              << std::setw(15) << mean_allocation_duration.count()
              << std::setw(15) << mean_deallocation_duration.count() << "    "
              << std::setw(9) << allocator_with_stats.get_stats().total[alloc_stats_type::Stats_Data::Allocation] << "  "
              << std::setw(9) << allocator_with_stats.get_stats().total[alloc_stats_type::Stats_Data::Failed_Allocation] << "    "
              << std::setw(10) << currently_allocated_size
              << "\n";
}


template <std::size_t BlockSize, std::size_t Levels, class LevelType = unsigned int, class IndexType = unsigned int>
auto make_buddy_alloc2(std::size_t managed_size)
{
    using blk_addr_type = dd99_allocators_namespace::block_allocator::buddy_namespace::buddy_block_address<LevelType, IndexType>;
    using raii_block_type = dd99_allocators_namespace::raii_block<>;
    using layout_type = dd99_allocators_namespace::block_allocator::buddy_namespace::buddy_standard_layout<blk_addr_type, BlockSize, Levels, BlockSize << (Levels-1), raii_block_type>;
    using traits_type = dd99_allocators_namespace::block_allocator::buddy_namespace::buddy_intrusive_state_traits<layout_type>;
    using state_type = dd99_allocators_namespace::block_allocator::buddy_namespace::buddy_intrusive_state<layout_type, raii_block_type>;

    constexpr std::size_t managed_alignment = layout_type::last_level_alignment;
    std::byte * managed_ptr = reinterpret_cast<std::byte *>(::operator new(managed_size, std::align_val_t{managed_alignment}));
    raii_block_type managed_block{
        dd99_allocators_namespace::block{.base = managed_ptr, .size = managed_size},
        [](dd99_allocators_namespace::block blk){ if(blk.base != nullptr) ::operator delete(blk.base, std::align_val_t{managed_alignment}); }
    };

    layout_type layout{std::move(managed_block)};

    auto state_size = traits_type::get_state_size(layout);
    constexpr auto state_alignment = traits_type::get_state_alignment();
    std::byte * state_ptr = reinterpret_cast<std::byte *>(::operator new(state_size, std::align_val_t{state_alignment}));
    raii_block_type state_block{
        dd99_allocators_namespace::block{.base = state_ptr, .size = state_size},
        [](dd99_allocators_namespace::block blk){ if(blk.base != nullptr) ::operator delete(blk.base, std::align_val_t{state_alignment}); }
    };

    auto state = traits_type::make_state(std::move(layout), std::move(state_block));

    return dd99_allocators_namespace::block_allocator::buddy{std::move(state)};
}

template <std::size_t BlockSize, std::size_t Levels, class LevelType = unsigned int, class IndexType = unsigned int>
auto make_buddy_alloc(std::size_t managed_size)
{
    using blk_addr_type = dd99_allocators_namespace::block_allocator::buddy_namespace::buddy_block_address<LevelType, IndexType>;
    using raii_block_type = dd99_allocators_namespace::raii_block<>;
    using layout_type = dd99_allocators_namespace::block_allocator::buddy_namespace::buddy_standard_layout<blk_addr_type, BlockSize, Levels, BlockSize << (Levels-1), raii_block_type>;
    using traits_type = dd99_allocators_namespace::block_allocator::buddy_namespace::buddy_fused_state_traits<layout_type>;
    using state_type = dd99_allocators_namespace::block_allocator::buddy_namespace::buddy_fused_state<layout_type, raii_block_type>;

    constexpr std::size_t managed_alignment = layout_type::last_level_alignment;
    std::byte * managed_ptr = reinterpret_cast<std::byte *>(::operator new(managed_size, std::align_val_t{managed_alignment}));
    raii_block_type managed_block{
        dd99_allocators_namespace::block{.base = managed_ptr, .size = managed_size},
        [](dd99_allocators_namespace::block blk){ if(blk.base != nullptr) ::operator delete(blk.base, std::align_val_t{managed_alignment}); }
    };

    layout_type layout{std::move(managed_block)};

    auto state_size = traits_type::get_state_size(layout);
    constexpr auto state_alignment = traits_type::get_state_alignment();
    std::byte * state_ptr = reinterpret_cast<std::byte *>(::operator new(state_size, std::align_val_t{state_alignment}));
    raii_block_type state_block{
        dd99_allocators_namespace::block{.base = state_ptr, .size = state_size},
        [](dd99_allocators_namespace::block blk){ if(blk.base != nullptr) ::operator delete(blk.base, std::align_val_t{state_alignment}); }
    };

    auto state = traits_type::make_state(std::move(layout), std::move(state_block));

    return dd99_allocators_namespace::block_allocator::buddy{std::move(state)};
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

    // using buddy_blk_address_type = alloc::buddy_namespace::buddy_block_address<>;
    // using buddy_layout_type = alloc::buddy_namespace::buddy_standard_layout<buddy_blk_address_type, 64, 11>;
    // using buddy_state_traits = alloc::buddy_namespace::buddy_intrusive_state_traits<buddy_layout_type>;
    // using buddy_state_type = alloc::buddy_namespace::buddy_intrusive_state<buddy_layout_type, mem::>;
    // using allocator_type = alloc::buddy<buddy_state_type>;

    // constexpr std::size_t mem_size = 1 << 20;
    // mem::self_contained_block<mem_size> memory_ac;
    // auto memory = memory_ac.get_block();

    // buddy_layout_type layout{memory};

    // auto aux_mem_size = buddy_state_traits::get_state_size(layout);
    // auto aux_mem_alignment = buddy_state_traits::get_state_alignment();

    // auto aux_memory_ptr = std::make_unique<std::byte[]>(aux_mem_size);
    // // assume alignment is enough
    // // TODO: ensure alignment is enough
    // mem::block aux_memory{aux_memory_ptr.get(), aux_mem_size};

    // auto buddy_state = buddy_state_traits::make_state(std::move(layout), aux_memory);
    // alloc::buddy allocator{std::move(buddy_state)};

    auto allocator = make_buddy_alloc<64, 11>(1 << 20);

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
