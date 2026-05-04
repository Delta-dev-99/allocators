#pragma once

#include <allocators/allocator.hpp>
#include <allocators/internal/structures/bitmap.hpp>

// #include <limits>
// #include <bit>
// #include <climits>
#include <algorithm>

// TODO: Read Roaring Bitmaps

namespace dd99::memory::block_allocator
{
    // Allocates fixed-size blocks, tracking free/used state with an internal bitmap.
    //
    // Template parameters:
    //   Block_Size:       size in bytes of each allocated block.
    //   Block_Alignment:  alignment guarantee for every returned block. Must be a
    //                     power of 2 and a divisor of Block_Size (so that every
    //                     block index preserves the alignment, since block N is at
    //                     blocks_base + N*Block_Size and Block_Alignment | Block_Size).
    //                     Defaults to Block_Size.
    //   Bitmap_Element_T: unsigned integer type used as the bitmap word.
    //
    // Memory layout is determined dynamically at construction. Two candidates are tried
    // for each block count N:
    //   Layout A – [bmp_padding?][bitmap][block_padding?][blocks]
    //   Layout B – [block_padding?][blocks][bmp_padding?][bitmap]
    // The layout that accommodates the most blocks wins. When both accommodate the
    // same N, Layout A is preferred (arbitrary tie-break).
    //
    // This two-candidate strategy maximises the probability of finding a valid
    // alignment boundary within the available padding, compared to fixing one layout.
    template <std::size_t Block_Size,
              std::size_t Block_Alignment  = Block_Size,
              class       Bitmap_Element_T = std::uint8_t>
    class Bitmap
    {
        using BMP = dd99::memory::structure::Bitmap<Bitmap_Element_T>;

        static_assert(Block_Size > 0,
            "Block_Size must be greater than 0");
        static_assert(Block_Alignment > 0,
            "Block_Alignment must be greater than 0");
        static_assert((Block_Alignment & (Block_Alignment - 1)) == 0,
            "Block_Alignment must be a power of 2");
        static_assert(Block_Size % Block_Alignment == 0,
            "Block_Size must be a multiple of Block_Alignment; "
            "this ensures every block index preserves the alignment guarantee");
        static_assert(std::is_unsigned_v<Bitmap_Element_T>,
            "Bitmap_Element_T must be an unsigned integer type");

    public:
        // Resolved memory arrangement for a given block count.
        // Exposed publicly to allow pre-construction introspection and unit testing.
        struct Layout
        {
            std::size_t  block_count = 0;
            std::byte  * blocks_base = nullptr; // nullptr == invalid (nothing fits)
            std::byte  * bmp_base    = nullptr;

            // A Layout is valid when blocks_base is non-null.
            // Note: block_count == 0 with a non-null blocks_base is a valid
            // (though degenerate) result for very small memory regions.
            bool valid() const noexcept { return blocks_base != nullptr; }
        };

    private:
        // Two-stage construction: the layout is fully resolved before any member
        // is initialised, avoiding any chicken-and-egg ordering issues.
        Bitmap(const memory::Block & memory, const Layout & layout)
            : m_memory{memory}
            , m_block_count{layout.block_count}
            , m_blocks_base{layout.blocks_base}
            , m_bitmap{layout.block_count, layout.bmp_base}
        {
            deallocate_all();
        }

    public:
        explicit Bitmap(const memory::Block & memory)
            : Bitmap(memory, find_layout(memory))
        { }

        Bitmap(const Bitmap &) = delete;
        Bitmap & operator=(const Bitmap &) = delete;

        Bitmap(Bitmap &&) = default;
        Bitmap & operator=(Bitmap &&) = default;

    public:
        // Allocate one block.
        //
        // Returns an empty Block if:
        //   - requested_size > Block_Size  (too large for this allocator)
        //   - requested_alignment > Block_Alignment  (guarantee unachievable)
        //   - no free block remains
        //
        // requested_alignment must be a power of 2; behaviour for other values
        // is undefined (matching the C++ standard's own precondition on alignment).
        [[nodiscard]]
        memory::Block allocate(std::size_t requested_size,
                               std::size_t requested_alignment = Block_Alignment)
        {
            if (requested_size      > Block_Size)      return {};
            if (requested_alignment > Block_Alignment) return {};

            const auto free_index = m_bitmap.set_first_unset();
            if (free_index != std::size_t(-1))
                return get_memory_block(free_index);

            return {};
        }

        // The caller must pass back the exact Block returned by allocate().
        // Passing a block not owned by this allocator is a no-op.
        void deallocate(const memory::Block & blk)
        {
            if (!owns(blk)) return;
            m_bitmap.unset(get_index(blk));
        }

        void deallocate_all()
        {
            m_bitmap.reset();
        }

        // Containment check. Note: only checks that blk lies within the overall
        // memory range; does not verify block alignment or exact block size.
        // Callers must always pass back the exact Block returned by allocate().
        bool owns(const memory::Block & blk) const noexcept
        {
            return m_memory.contains(blk);
        }

        bool owns(const std::byte * ptr) const noexcept
        {
            return m_memory.contains(ptr);
        }

    public:
        // Determine the best layout for `memory`.
        //
        // Binary-searches for the maximum N such that N blocks plus their bitmap
        // fit inside `memory` under at least one of the two candidate layouts.
        // "N blocks fit" is monotonically non-increasing in N, so binary search
        // is correct and terminates.
        //
        // Returns a Layout with blocks_base == nullptr only if even the degenerate
        // 0-block arrangement cannot satisfy alignment constraints (extremely small
        // or misaligned memory). In that case the resulting allocator has no usable
        // blocks.
        static constexpr Layout find_layout(const memory::Block & memory)
        {
            // Round ptr up to the nearest multiple of `alignment` (must be a power of 2).
            auto align_up = [](std::byte * ptr, std::size_t alignment) -> std::byte *
            {
                const auto u = reinterpret_cast<std::uintptr_t>(ptr);
                return reinterpret_cast<std::byte *>((u + alignment - 1) & ~(alignment - 1));
            };

            // Bytes consumed by the bitmap when tracking `n` blocks.
            auto bitmap_bytes = [](std::size_t n) -> std::size_t
            {
                return BMP::Block_Size * BMP::calculate_block_count(n);
            };

            // Try to fit `n` blocks inside `memory`.
            // Attempts Layout A first, then Layout B.
            // Returns an invalid Layout (blocks_base == nullptr) if neither fits.
            auto calc_layout = [&](std::size_t n) -> Layout
            {
                const std::size_t bmp_bytes = bitmap_bytes(n);
                const std::size_t blk_bytes = n * Block_Size;

                // Layout A: [bmp_padding?][bitmap][block_padding?][blocks]
                // The bitmap sits as early as possible; blocks follow after the
                // required alignment gap.
                {
                    std::byte * bmp_base = align_up(memory.base,          BMP::Block_Alignment);
                    std::byte * blk_base = align_up(bmp_base + bmp_bytes, Block_Alignment);
                    if (blk_base + blk_bytes <= memory.get_end())
                        return {n, blk_base, bmp_base};
                }

                // Layout B: [block_padding?][blocks][bmp_padding?][bitmap]
                // The blocks sit as early as possible; the bitmap follows.
                // This gives a different alignment anchor point than Layout A,
                // improving the chance of absorbing awkward alignment gaps.
                {
                    std::byte * blk_base = align_up(memory.base,          Block_Alignment);
                    std::byte * bmp_base = align_up(blk_base + blk_bytes, BMP::Block_Alignment);
                    if (bmp_base + bmp_bytes <= memory.get_end())
                        return {n, blk_base, bmp_base};
                }

                return {}; // blocks_base == nullptr signals "does not fit"
            };

            const std::size_t max_n = memory.size / Block_Size; // absolute upper bound
            
            // Initialise `best` with the N=0 base case.
            // calc_layout(0) can only fail for degenerate (extremely small or
            // misaligned) memory; in that case best stays invalid.
            Layout      best = calc_layout(0);
            std::size_t lo   = 0;
            std::size_t hi   = max_n;

            // Binary search for the largest N that fits.
            while (lo < hi)
            {
                // Ceiling midpoint: ensures progress when hi == lo + 1,
                // since mid would equal hi (not lo), changing the interval
                // regardless of which branch is taken.
                const std::size_t mid       = lo + ((hi - lo + 1) >> 1);
                const Layout      candidate = calc_layout(mid);

                if (candidate.valid())
                {
                    best = candidate; // track the best layout seen so far,
                    lo   = mid;       // avoiding a redundant call after the loop
                }
                else
                {
                    hi = mid - 1;
            }
            }

            return best;
        }

    private:
        memory::Block  m_memory;
        std::size_t    m_block_count;
        std::byte *    m_blocks_base;
        BMP            m_bitmap;

        Block get_memory_block(std::size_t index) const
        {
            return {.base = m_blocks_base + Block_Size * index, .size = Block_Size};
        }

        std::size_t get_index(const memory::Block & blk) const
        {
            return std::size_t(blk.base - m_blocks_base) / Block_Size;
        }
    };

    static_assert(Block_Allocator<Bitmap<1>>,
        "Bitmap does not satisfy the Block_Allocator concept");

} // namespace dd99::memory::block_allocator
