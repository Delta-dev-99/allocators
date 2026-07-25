#pragma once

// =============================================================================
// buddy_fused_state.hpp
// =============================================================================
//
// TODO: *** AI-Generated. Review this
// 
// A non-intrusive buddy state that fuses free-list linkage and block status
// into a single per-block descriptor array — inspired by Linux's physical page
// allocator (mm/page_alloc.c, struct page / struct free_area).
//
//
// DESIGN RATIONALE
// ----------------
// The intrusive state (buddy_intrusive_state) stores linked-list nodes inside
// the managed blocks themselves, and records buddy-pair parity in a separate
// external XOR bitmap.  That design is excellent for raw throughput (the list
// node is right there in the block, no extra indirection), but it has costs:
//
//   1. It is *intrusive*: it writes bookkeeping data into managed memory.
//      This is unacceptable when managed memory must be kept clean (e.g.
//      physical pages that should be zeroed on allocation, memory-mapped I/O
//      regions, or configurations where a use-after-free sanitiser watches the
//      managed range).
//
//   2. The XOR bitmap and the free list are two separate data structures, so
//      processing a block on allocation or deallocation may touch two distinct
//      cache lines: one in the block being allocated/freed (the list node) and
//      one in the bitmap.
//
// The fused state solves both problems by keeping a flat array of small
// per-block descriptors — called block_slots — in the *state* memory rather
// than in the managed memory.  Each slot stores:
//
//     next     — base-level index of the next free block at the same level
//     prev     — base-level index of the previous free block (NONE if head)
//     level    — the level at which this block is currently free
//     is_free  — whether this slot heads a free block in a free list
//
// Because the list links and the allocation state live in the same struct,
// a single array access provides *all* information needed for a push, pop, or
// merge decision.  This is the "fused" property.
//
// The XOR bitmap is eliminated entirely.  Instead of toggling a shared parity
// bit, we inspect the buddy's slot directly: if buddy.is_free && buddy.level
// == our_level, the buddy is free at the right granularity and we can merge.
// Two reasons the level check is necessary:
//
//   • The buddy slot might be the representative of a *larger* free block
//     (buddy was merged upward while we were allocated).
//   • The buddy slot might be free at a *smaller* level (buddy was split
//     downward after we were allocated).
//
//
// SLOT INDEXING
// -------------
// Slots are indexed by *base-level block index* — the index a block would have
// if the whole memory were a flat array of the smallest (level-0) blocks.
//
// For a block at (level L, index k):
//
//     base_index = k * (1 << L)
//
// This is also the index of the block's leading base-level block, and the
// slot at that index is the block's *representative* slot — the one that holds
// its is_free / level / next / prev state.  Slots for the other base-level
// blocks that make up the same higher-level block are unused while that block
// is free; they only become active if the block is split.
//
//
// BUDDY INDEX ARITHMETIC
// ----------------------
// Within a level, blocks pair up as (0,1), (2,3), (4,5), …  Block k's buddy
// is k ^ 1.  Their base-level indices are:
//
//     idx        = k     * (1 << L)
//     buddy_idx  = (k^1) * (1 << L)  =  idx ^ (1 << L)
//
// Flipping bit L of the base-level index is the whole buddy computation.
//
//
// MEMORY OVERHEAD
// ---------------
// For N base-level blocks, the state requires N * sizeof(block_slot) bytes.
// With 32-bit index_type and level_type = uint8_t that is typically 10 bytes
// per slot, padded to 12.  For a 4 KiB base block size this is about 0.3%.
//
//
// PERFORMANCE (all O(1))
// ----------------------
//   push          : writes own slot; updates old head's prev link (1–2 slots)
//   pop           : reads head index; unlinks it (1–2 slots)
//   merge_or_push : reads own slot + buddy slot (2 slots, fixed offset)
//
// The buddy slot offset (idx ^ (1<<L)) is fully predictable, so the hardware
// prefetcher can pipeline both reads at small levels where they are hot.
//
// =============================================================================

#include <allocators/block_allocators/basic/buddy/buddy_layout.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_state.hpp>
#include <algorithm>    // std::min, std::fill
#include <array>
#include <cstddef>
#include <limits>       // std::numeric_limits
#include <memory>       // std::uninitialized_fill
#include <span>


namespace dd99_allocators_namespace::block_allocator::buddy_namespace
{

    // Forward declaration so the traits factory function can name the state type.
    template <Layout_Concept Layout, class State_Memory_Block_Type>
    struct buddy_fused_state;


    // =========================================================================
    // buddy_fused_state_traits
    // =========================================================================
    // Mirrors the interface of buddy_intrusive_state_traits.  Provides:
    //   - block_slot        : the per-block descriptor type
    //   - get_state_size    : bytes of state memory required
    //   - get_state_alignment
    //   - make_state        : factory that constructs the state object
    //
    template <Layout_Concept Layout>
    struct buddy_fused_state_traits
    {
        using layout_type        = Layout;
        using block_address_type = layout_type::block_address_type;
        using level_type         = block_address_type::level_type;
        using index_type         = block_address_type::index_type;


        // ---------------------------------------------------------------------
        // block_slot — the "fused" per-block descriptor
        // ---------------------------------------------------------------------
        // One instance lives in the state array for every base-level block.
        // Only the *representative* slot (the one at the block's leading
        // base-level index) is populated while a block is free.  The remaining
        // slots covered by that block are dormant until a split activates them.
        //
        struct block_slot
        {
            // Sentinel: "no neighbour / empty list".
            // The maximum value of index_type is safe because a managed region
            // large enough to have that many base-level blocks would exceed the
            // representable address space anyway.
            static constexpr index_type NONE = std::numeric_limits<index_type>::max();

            // Doubly-linked free-list links, stored as base-level indices rather
            // than pointers.  Index-based links stay valid regardless of where
            // the state array itself is located in virtual memory.
            index_type next    = NONE;   // next free block at the same level (NONE = tail)
            index_type prev    = NONE;   // prev free block at the same level (NONE = head)

            // The level at which this block is currently free.
            // Meaningful only when is_free == true.
            level_type level   = 0;

            // True iff this slot is the representative of a free block that is
            // linked into a free list.
            bool       is_free = false;
        };


        // Total bytes the caller must supply as state memory.
        static constexpr
        std::size_t
        get_state_size(const layout_type & layout)
        {
            return static_cast<std::size_t>(layout.m_block_count) * sizeof(block_slot);
        }

        // Required alignment of the state memory block.
        static constexpr
        std::size_t
        get_state_alignment()
        {
            return alignof(block_slot);
        }

        // Factory: builds a buddy_fused_state from a prepared layout and a
        // state memory block of at least get_state_size() bytes, aligned to
        // at least get_state_alignment() bytes.
        template <class State_Memory_Block_Type>
        [[nodiscard]]
        static constexpr
        auto
        make_state(layout_type && layout, State_Memory_Block_Type && state_block)
            -> buddy_fused_state<layout_type, std::decay_t<State_Memory_Block_Type>>
        {
            return {std::move(layout), std::forward<State_Memory_Block_Type>(state_block)};
        }
    };


    // =========================================================================
    // buddy_fused_state
    // =========================================================================
    template <Layout_Concept Layout, class State_Memory_Block_Type>
    struct buddy_fused_state
    {
        // Uncomment to verify this type satisfies the State_Concept interface:
        // static_assert(State_Concept<buddy_fused_state>);

        using layout_type             = Layout;
        using state_memory_block_type = State_Memory_Block_Type;
        using block_address_type      = layout_type::block_address_type;
        using level_type              = block_address_type::level_type;
        using index_type              = block_address_type::index_type;
        using traits_type             = buddy_fused_state_traits<layout_type>;
        using block_slot              = traits_type::block_slot;

        static constexpr auto levels     = layout_type::levels;
        static constexpr auto last_level = levels - 1;
        static constexpr auto block_size = layout_type::block_size;


        // ----- construction --------------------------------------------------

        // The constructor is not constexpr because it uses reinterpret_cast to
        // overlay the slot array onto the raw state memory.
        buddy_fused_state(layout_type layout, state_memory_block_type state_memory)
            : m_layout      {std::move(layout)}
            , m_state_memory{std::move(state_memory)}
            // , m_slots       {reinterpret_cast<block_slot *>(m_state_memory.get_base()),
            //                  static_cast<std::size_t>(m_layout.m_block_count)}
        {
            // The state memory is raw uninitialised bytes; construct all slots
            // in place before we start using them.
            auto slots = get_slots();
            std::uninitialized_fill(slots.begin(), slots.end(), block_slot{});

            // All free-list heads start empty.
            m_heads.fill(block_slot::NONE);

            init_freelists();
        }


        // ----- State_Concept interface ----------------------------------------

        // Mark block_base as free at `level` and prepend it to that level's
        // free list.
        constexpr
        void
        push(level_type level, std::byte * block_base)
        {
            index_type  idx  = to_base_index(block_base);
            auto slots = get_slots();
            block_slot & slot = slots[idx];

            // Populate the representative slot.
            slot.level   = level;
            slot.is_free = true;

            // Prepend to the doubly-linked free list for `level`.
            index_type old_head = m_heads[level];
            slot.next = old_head;
            slot.prev = block_slot::NONE;      // new head has no predecessor

            if (old_head != block_slot::NONE)
                slots[old_head].prev = idx;  // back-link the former head

            m_heads[level] = idx;
        }

        // Take the head free block from `level`'s list, mark it allocated, and
        // return its base address.  Returns nullptr if the list is empty.
        [[nodiscard]]
        constexpr
        std::byte *
        pop(level_type level)
        {
            index_type head = m_heads[level];
            if (head == block_slot::NONE) return nullptr;

            unlink(head);
            return to_block_base(head);
        }

        // Called during deallocation with a block that has just been freed.
        //
        // This is the heart of the buddy algorithm.  We check whether the
        // buddy block is also free *at the same level*.  Two outcomes:
        //
        //   (a) Buddy is free at the same level → MERGE.
        //       Remove the buddy from its free list.  Return the base address
        //       of the merged (joint) block — always the lower of the two
        //       addresses — so the caller can recurse one level up.
        //
        //   (b) Buddy is not free, or is free at a different level → PUSH.
        //       Add this block to the free list and return nullptr, ending
        //       the upward merge chain.
        //
        // The buddy.level == level check in case (a) is essential: without it
        // we might incorrectly merge with a buddy that has been split into
        // smaller pieces or merged into a larger block.
        //
        [[nodiscard]]
        constexpr
        std::byte *
        merge_or_push(level_type level, std::byte * block_base)
        {
            // Blocks without a buddy (lone top-level block, or an orphaned
            // lower-level block at the end of a non-power-of-2 memory region)
            // go straight onto the free list.
            block_address_type block_addr = m_layout.get_block_address(block_base, level);
            if (!m_layout.block_has_buddy(block_addr))
            {
                push(level, block_base);
                return nullptr;
            }

            // -----------------------------------------------------------------
            // Buddy index arithmetic:
            //
            //   idx       = k     * (1 << level)     (our base-level index)
            //   buddy_idx = (k^1) * (1 << level)
            //             = k * (1 << level) ^ (1 << level)
            //             = idx ^ (1 << level)
            //
            // Flipping bit `level` of the base-level index gives the buddy's
            // base-level index.  One array read from buddy_idx gives us both
            // the buddy's free status and its current level — the "fused" part.
            // -----------------------------------------------------------------
            index_type idx       = to_base_index(block_base);
            index_type buddy_idx = idx ^ (static_cast<index_type>(std::size_t{1} << level));

            const block_slot & buddy = get_slots()[buddy_idx];

            if (buddy.is_free && buddy.level == level)
            {
                // Buddy is free at the right granularity: merge.
                unlink(buddy_idx);
                // The merged block starts at the lower of the two addresses.
                return to_block_base(std::min(idx, buddy_idx));
            }

            // Buddy is busy or at the wrong level: just push.
            push(level, block_base);
            return nullptr;
        }

        // Reset to the same state as right after construction.
        constexpr
        void
        reset()
        {
            // Objects are already constructed here, so plain assignment is fine.
            auto slots = get_slots();
            std::fill(slots.begin(), slots.end(), block_slot{});
            m_heads.fill(block_slot::NONE);
            init_freelists();
        }


    private: // ----- helpers --------------------------------------------------

        // Convert a block's base pointer to its base-level index.
        // block_size is always a power of 2, so the compiler reduces the
        // division to a right shift.
        constexpr
        index_type
        to_base_index(const std::byte * block_base) const
        {
            return static_cast<index_type>(
                (block_base - m_layout.m_memory.get_base()) / block_size);
        }

        // Convert a base-level index to the corresponding block base pointer.
        constexpr
        std::byte *
        to_block_base(index_type idx) const
        {
            return m_layout.m_memory.get_base() + static_cast<std::size_t>(idx) * block_size;
        }

        // Remove slot `idx` from whatever free list it currently belongs to,
        // and mark it as allocated.
        // Precondition: get_slots()[idx].is_free == true.
        constexpr
        void
        unlink(index_type idx)
        {
            block_slot & slot = get_slots()[idx];

            // Patch the predecessor's forward link (or the list head if we
            // are the head, i.e. prev == NONE).
            if (slot.prev != block_slot::NONE)
                get_slots()[slot.prev].next = slot.next;
            else
                m_heads[slot.level] = slot.next;

            // Patch the successor's backward link (if any).
            if (slot.next != block_slot::NONE)
                get_slots()[slot.next].prev = slot.prev;

            // Clear the slot so stale data cannot mislead a future lookup
            // of this slot as a buddy.
            slot.is_free = false;
            slot.next    = block_slot::NONE;
            slot.prev    = block_slot::NONE;
        }

        // Populate free lists from scratch.
        //
        // Strategy (same as buddy_intrusive_state::init_freelists):
        //
        //   1. All top-level (last_level) blocks go in as free.  They are the
        //      largest possible chunks and subsume everything below them.
        //
        //   2. At each lower level, if the last block at that level has no buddy
        //      it is "orphaned" — it cannot be represented by a higher-level
        //      entry because the managed region is not a power-of-2 multiple of
        //      the top-level block size.  Such orphaned blocks are pushed
        //      directly into their own level's free list.
        //
        // Together, steps 1 and 2 produce a partition of the managed memory into
        // non-overlapping, non-adjacent free blocks, one per "bit" in the binary
        // representation of the total base-level block count.
        //
        constexpr
        void
        init_freelists()
        {
            // Step 1: top-level blocks.
            index_type last_count = m_layout.get_level_block_count(last_level);
            for (index_type i = 0; i < last_count; ++i)
                push(last_level, m_layout.get_block({.level = last_level, .index = i}).base);

            // Step 2: orphaned lower-level blocks.
            for (level_type lv = 0; lv < last_level; ++lv)
            {
                index_type count = m_layout.get_level_block_count(lv);
                if (count == 0) break;  // higher levels are even smaller: early exit

                block_address_type last_addr{.level = lv, .index = count - 1};
                if (!m_layout.block_has_buddy(last_addr))
                    push(lv, m_layout.get_block(last_addr).base);
            }
        }


    public:
        // Public for the same reason buddy_intrusive_state's members are public:
        // buddy<> needs to reach m_layout.m_memory for owns() checks.

        layout_type              m_layout;
        state_memory_block_type  m_state_memory;

        // Non-owning view of the slot array that lives inside m_state_memory,
        // indexed by base-level block index.
        // std::span<block_slot>    m_slots;
        constexpr std::span<block_slot> get_slots() const
        {
            return {std::launder(reinterpret_cast<block_slot *>(m_state_memory.get_base())),
                    static_cast<std::size_t>(m_layout.m_block_count)};
        }

        // Per-level free-list heads: m_heads[L] is the base-level index of the
        // first free block at level L, or block_slot::NONE if that list is
        // empty.  Kept inline (not in state memory) because it is tiny and is
        // touched on every single allocator operation.
        std::array<index_type, levels> m_heads;
    };

} // namespace dd99_allocators_namespace::block_allocator::buddy_namespace


// =============================================================================
// Example: how to construct and use a fused state
// =============================================================================
//
// auto layout = make_buddy_layout</*block_size=*/64, /*levels=*/6>(main_memory);
//
// using traits = buddy_fused_state_traits<decltype(layout)>;
// auto sz      = traits::get_state_size(layout);      // bytes needed
// auto align   = traits::get_state_alignment();        // required alignment
//
// auto state_mem = allocate_state_memory_somehow(sz, align);
//
// auto state = traits::make_state(std::move(layout), std::move(state_mem));
//
// // Hand the state to the generic buddy allocator:
// auto allocator = buddy{std::move(state)};
//
// // Everything else is identical to the intrusive variant.
// auto blk = allocator.allocate(256);
// allocator.deallocate(blk);