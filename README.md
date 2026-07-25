# dd99::memory — composable, policy-free C++26 allocators

A header-only C++26 library for building exactly the memory allocator you need, one
composable piece at a time — from user-space object pools to freestanding kernel arenas.

![C++26](https://img.shields.io/badge/C%2B%2B-26-blue.svg)
![Header Only](https://img.shields.io/badge/header--only-yes-brightgreen.svg)
![Status](https://img.shields.io/badge/status-pre--1.0-orange.svg)

No hidden allocations. No global state. No exceptions or RTTI required in the core
allocation path. Every allocator is handed exactly the memory it's allowed to touch —
including its own bookkeeping memory — and nothing more.

---

## Table of Contents

- [Why this library](#why-this-library)
- [30-second example](#30-second-example)
- [Getting started](#getting-started)
- [Core concepts](#core-concepts)
- [A tour of the allocators](#a-tour-of-the-allocators)
- [Usage examples](#usage-examples)
- [Freestanding & kernel support](#freestanding--kernel-support)
- [Design philosophy](#design-philosophy)
- [Project status & roadmap](#project-status--roadmap)
- [Comparison & inspiration](#comparison--inspiration)
- [Testing & quality](#testing--quality)
- [Contributing](#contributing)
- [Support this project](#support-this-project)
- [License](#license)

---

## Why this library

Most allocator libraries pick a lane: general-purpose (`malloc`), STL-integrated
(`std::pmr`), or embedded/kernel-only (write-your-own). `dd99::memory` works across all
three, by refusing to assume anything about the environment it runs in:

- **You always control the memory.** The allocators built into this library never call
  the OS or the heap on their own — you construct them with the memory blocks they
  operate on (managed memory *and* bookkeeping memory, where applicable). That means
  they work equally well over a `std::byte[]` on the stack, a `mmap`'d region, or raw
  physical pages in a kernel. `Block_Allocator` is a structural concept, not a base
  class, so nothing stops you from writing your own allocator that wraps `malloc` if a
  given situation calls for it.
- **No allocation headers by default.** The block allocators in this library return a
  `{pointer, size}` pair instead of a bare pointer, so there's no need for a header
  hidden next to the returned pointer, and you always know exactly how much memory you
  actually got. This is not a rule the concept enforces — the pointer-allocator bridge
  types (`Basic`, `Pointer_Checked`) deliberately use a header, to present a familiar
  `malloc`/`free`-style pointer API — it's simply not needed by the core block
  allocators, and it's easy to add back where it earns its keep.
- **Composition is free.** Wrapping an allocator with statistics, a fallback strategy,
  or a size threshold is template composition — no vtables unless you explicitly ask for
  one (`any_block_allocator_ref`, for when you need a real runtime boundary).
- **Nothing is implicit.** Storage sizing, alignment, and error handling are all
  explicit and checkable at the call site. If you've ever wanted to know *exactly* what
  an allocator does with your memory, this library is built for you.

## 30-second example

```cpp
#include <allocators/structures/blocks/self_contained_block.hpp>
#include <allocators/block_allocators/basic/stack/stack.hpp>

int main()
{
    dd99::memory::self_contained_block<4096> memory;   // 4 KiB, lives wherever this variable lives
    dd99::memory::block_allocator::Stack stack{memory.get_block()};

    auto blk = stack.allocate(128, alignof(std::max_align_t));
    // ... use blk.base, blk.size ...
    stack.deallocate(blk);                             // stack allocators can only free the top block
}
```

## Getting started

### CMake

```cmake
include(FetchContent)
FetchContent_Declare(
    dd99_allocators
    GIT_REPOSITORY https://github.com/<your-username>/allocators.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(dd99_allocators)

target_link_libraries(your_target PRIVATE dd99::allocators)
```

Or, if you're vendoring the repo directly: `add_subdirectory(external/allocators)` and
link against `dd99::allocators`.

### Bazel (bzlmod)

```python
# MODULE.bazel
bazel_dep(name = "dd99_allocators", version = "2.0.0")
git_override(
    module_name = "dd99_allocators",
    remote = "https://github.com/<your-username>/allocators.git",
    commit = "<commit-hash>",
)
```

```python
# BUILD
cc_binary(
    name = "your_target",
    srcs = ["main.cpp"],
    deps = ["@dd99_allocators//allocators"],
)
```

### Conan

```ini
[requires]
allocators/2.0.0
```

The recipe is a plain `header-library` package (see `conanfile.py`) — export it to your
local cache with `conan export .` until it's published to a remote.

NOTE: Conan support hasn't been maintained.

### Just the headers

It's header-only. Copy `allocators/include/allocators` onto your include path and:

```cpp
#include <allocators/allocators.hpp>
```

Note that the umbrella header is intentionally *not* exhaustive — see
[Freestanding & kernel support](#freestanding--kernel-support) for what's opt-in and why.

### Requirements

- A compiler with C++26 support and `-std=c++26`. Developed and tested with **GCC 14+**;
  other compilers are currently untested.
- CMake ≥ 3.30, or Bazel with bzlmod enabled, if you're not vendoring headers directly.

## Core concepts

### The two-value paradigm

Every allocation returns a `block` — a `{pointer, size}` pair — instead of a bare
pointer, and the same value must be handed back on deallocation:

```cpp
struct block
{
    std::byte * base = nullptr;
    std::size_t size = 0;
};
```

This is the central design decision the whole library is built around. It removes the
need for an allocation header hidden just before the returned pointer, which means a
buffer overrun can corrupt a local variable but can't corrupt allocator bookkeeping. It
also means the size you get back is information you actually have, which fixed-size and
slab-style allocators rely on for speed. The trade-off is that user code has to carry
the size around — `raii_block<Deallocator>` (unique_ptr-like, move-only,
deallocate-on-destruction) and `new_result<T>` (returned by `allocator_new`) exist to do
that for you when you want automatic lifetime management instead of manual bookkeeping.

### Composability without virtual dispatch

Any type that satisfies this concept is a block allocator, no base class
required:

```cpp
template <class Alloc>
concept Block_Allocator = requires(Alloc alloc, std::size_t size, std::size_t alignment,
                                    const block & blk, const std::byte * ptr)
{
    { alloc.allocate(size) }            -> std::same_as<block>;
    { alloc.allocate(size, alignment) } -> std::same_as<block>;
    { alloc.deallocate(blk) }           -> std::same_as<void>;
    { alloc.deallocate_all() }          -> std::same_as<void>;
    { alloc.owns(blk) }                 -> std::same_as<bool>;
    { alloc.owns(ptr) }                 -> std::same_as<bool>;
};
```

Composite allocators (`Fallback`, `Stats`, `Segregator`, ...) are themselves templates
over `Block_Allocator`, so they nest arbitrarily and the whole chain typically inlines
away — you only pay for a vtable when you explicitly use
[`any_block_allocator_ref`](#type-erasure-at-the-boundary).

### Configurable assertions

Instead of a single hard-coded `assert`, the library has four escalating tiers, each
independently toggleable, and a single user-overridable `[[noreturn]]` handler:

| Level | Meaning | Default |
|---|---|---|
| `NONE` | No checks | — |
| `CRITICAL` | Allocator state corruption / construction errors — dangerous to continue | on, always |
| `HARDENED` | API contract violations (caller bugs) — misaligned memory, bad alignment argument, etc. | off in `NDEBUG` builds |
| `DEBUG` | Internal invariant checks, too expensive for any production build | on outside `NDEBUG` |

```cpp
// override the default abort-and-print handler, e.g. to call your kernel's panic():
namespace dd99::memory
{
    void allocators_assertion_failed(const assertion_info & info) noexcept
    {
        panic("%.*s:%u: %.*s", (int)info.file.size(), info.file.data(),
              info.line, (int)info.message.size(), info.message.data());
    }
}
```

Tune the compiled-in tier with `-DDD99_ALLOCATORS_ASSERT_LEVEL=0..3`.

## A tour of the allocators

| Category | Types | What it does |
|---|---|---|
| **Basic** | `bitmap`, `buddy`, `Pool`, `Slicing`, `Stack` | Allocate directly from user-supplied memory |
| **Composite** | `Fallback`, `Filter`, `Quantizer`, `Ref`, `Segregator`, `Throwing` | Wrap one or more allocators to add behavior |
| **Degenerate** | `Boolean`, `Constant`, `Null` | Minimal-footprint allocators for restricted scenarios (single-block, always-fail, ...) |
| **Metrics** | `Stats`, `Timing` | Observe an allocator without changing its behavior |
| **Pointer bridge** | `Basic`, `Pointer_Checked` | `malloc`/`free`-style pointer API layered over a block allocator |
| **Utility** *(experimental)* | `Switch`, `Owner`, `XRef`, `raii_block_allocator` | Bend the allocator interface for specific needs — API not yet stable, excluded from the umbrella header |

> Earlier versions of this library had a separate `basic::Buddy` / `borrowing::Buddy`
> split. That's gone — lifetime and ownership customization now happens through the
> block *type* template parameter (pass `raii_block<>` instead of the default `block`
> where you want RAII), not through a parallel hierarchy of "borrowing" allocators.

## Usage examples

### Composing allocators

Composition is just nested construction — here's a freelist allocator with allocation
statistics bolted on, with no code changes to either piece:

```cpp
#include <allocators/allocators.hpp>
#include <allocators/block_allocators/basic/slicing/slicing.hpp>

namespace mem   = dd99::memory;
namespace alloc = mem::block_allocator;

int main()
{
    mem::self_contained_block<4096> pool;
    auto tracked = alloc::metrics::Stats(alloc::Slicing(pool.get_block()));

    auto a = tracked.allocate(64);
    auto b = tracked.allocate(128);
    tracked.deallocate(a);

    auto allocated = tracked.get_stats().total[decltype(tracked)::Stats_Data::Allocated_Size];
    tracked.deallocate(b);
}
```

Or a two-tier allocator that tries one pool and falls back to another, each with its own
stats, exposed through a checked pointer API:

```cpp
#include <allocators/allocators.hpp>
#include <allocators/block_allocators/basic/slicing/slicing.hpp>

namespace mem       = dd99::memory;
namespace alloc     = mem::block_allocator;
namespace ptr_alloc = mem::pointer_allocator;

mem::self_contained_block<256>  fast_pool;
mem::self_contained_block<4096> big_pool;

auto allocator = ptr_alloc::Pointer_Checked(
    alloc::composite::Fallback(
        alloc::metrics::Stats(alloc::Slicing(fast_pool.get_block())),
        alloc::metrics::Stats(alloc::Slicing(big_pool.get_block()))));

auto * p = allocator.allocate(200);
allocator.deallocate(p);
```

### Explicit bookkeeping storage

Some allocators need memory for their own state as well as the memory they hand out —
that memory is never allocated behind your back, you compute and provide it:

```cpp
#include <allocators/block_allocators/basic/bitmap/bitmap.hpp>
#include <allocators/structures/blocks/self_contained_block.hpp>
#include <vector>

using namespace dd99::memory;

int main()
{
    constexpr std::size_t block_size = 64;
    using bmp_alloc_type = block_allocator::bitmap<block_size>;

    self_contained_block<4096> managed;                                  // memory being handed out
    auto count = bmp_alloc_type::calculate_block_count(managed.get_block());

    using state_bitmap = structure::Bitmap<>;
    std::vector<std::byte> state(state_bitmap::calculate_block_count(count) * state_bitmap::block_size);

    bmp_alloc_type bmp{managed.get_block(), block{state.data(), state.size()}};

    auto blk = bmp.allocate(block_size);
    bmp.deallocate(blk);
}
```

### Object construction

`allocator_new` / `allocator_delete` bridge block allocators to typed object
construction (including array forms), with `new_result<T>` carrying the allocation
metadata so nothing gets lost:

```cpp
#include <allocators/structures/blocks/self_contained_block.hpp>
#include <allocators/block_allocators/basic/stack/stack.hpp>
#include <allocators/block_allocators/new.hpp>

struct Widget { explicit Widget(int id) : id(id) {} int id; };

int main()
{
    dd99::memory::self_contained_block<256> memory;
    auto stack = dd99::memory::block_allocator::Stack{memory.get_block()};

    auto widget = dd99::memory::allocator_new<Widget>(stack, /* id = */ 42);
    // widget->id == 42

    dd99::memory::allocator_delete(stack, widget);
}
```

(Plain `new`/`delete` expressions with instance-specific allocators are deliberately not
supported — there's no placement-delete expression in the language to make that safe.
See `pointer_allocators/new_expressions.md` for the full rationale.)

### Type erasure at the boundary

When you do need runtime polymorphism — e.g. crossing a shared-library boundary, or
storing heterogeneous allocators in a container — `any_block_allocator_ref` gives you a
non-owning, manually-vtabled wrapper that still satisfies `Block_Allocator`:

```cpp
#include <allocators/block_allocators/any_block_allocator.hpp>

void fill(dd99::memory::block_allocator::any_block_allocator_ref allocator)
{
    auto blk = allocator.allocate(64);
    // ...
    allocator.deallocate(blk);
}
```

### The buddy allocator

The buddy allocator is the flagship of the library and follows a deliberate three-layer
split: **layout** (pure geometry/addressing, `buddy_standard_layout`), **state**
(bookkeeping — pick an implementation), and **allocator** (`buddy<State_Type>`, which
coordinates the other two). This is more upfront ceremony than a single-template buddy
allocator, but it means the addressing scheme and the bookkeeping data structure vary
independently.

Two state implementations ship today:

| State implementation | Storage | Works over unmapped memory | Notes |
|---|---|---|---|
| `buddy_fused_state` | External per-block descriptor array (state memory) | Yes | Recommended default |
| `buddy_intrusive_state` | Freelist nodes in managed blocks + external XOR bitmap | No | Smaller memory footprint |

Since construction is explicit, a small factory function is the natural way to use it:

```cpp
#include <allocators/block_allocators/basic/buddy/buddy.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_layout.hpp>
#include <allocators/block_allocators/basic/buddy/buddy_block_address.hpp>
#include <allocators/block_allocators/basic/buddy/state_implementations/buddy_fused_state.hpp>
#include <allocators/structures/blocks/raii_block.hpp>
#include <new>

namespace mem      = dd99::memory;
namespace alloc    = mem::block_allocator;
namespace buddy_ns = alloc::buddy_namespace;

// Builds a buddy allocator that owns both its managed region and its bookkeeping state,
// using the non-intrusive "fused" state implementation.
template <std::size_t Block_Size, std::size_t Levels>
auto make_buddy(std::size_t managed_size)
{
    using addr_type    = buddy_ns::buddy_block_address<>;
    using owning_block = mem::raii_block<>;
    using layout_type  = buddy_ns::buddy_standard_layout<addr_type, Block_Size, Levels,
                                                          Block_Size << (Levels - 1), owning_block>;
    using traits_type  = buddy_ns::buddy_fused_state_traits<layout_type>;

    constexpr auto managed_alignment = layout_type::last_level_alignment;
    auto * managed_ptr = static_cast<std::byte *>(::operator new(managed_size, std::align_val_t{managed_alignment}));
    owning_block managed{
        mem::block{managed_ptr, managed_size},
        [](mem::block b) { ::operator delete(b.base, std::align_val_t{managed_alignment}); }
    };
    layout_type layout{std::move(managed)};

    auto state_size = traits_type::get_state_size(layout);
    constexpr auto state_alignment = traits_type::get_state_alignment();
    auto * state_ptr = static_cast<std::byte *>(::operator new(state_size, std::align_val_t{state_alignment}));
    owning_block state{
        mem::block{state_ptr, state_size},
        [](mem::block b) { ::operator delete(b.base, std::align_val_t{state_alignment}); }
    };

    return alloc::buddy{traits_type::make_state(std::move(layout), std::move(state))};
}

int main()
{
    auto allocator = make_buddy<64, 11>(1 << 20);   // 1 MiB arena, 64 B .. 64 KiB blocks

    auto blk = allocator.allocate(200);             // rounds up to the 256 B level
    allocator.deallocate(blk);
}
```

## Freestanding & kernel support

Kernel and freestanding use is a first-class target, not an afterthought:

- **No hidden heap allocation.** Every allocator — including the buddy allocator's
  bookkeeping state — is handed its memory explicitly by the caller.
- **Exceptions are opt-in.** The core allocation path never throws. Exceptions only
  appear in explicitly-chosen wrappers (`composite::Throwing`, `pointer_allocator::
  Pointer_Checked`'s corruption check) — leave them out and nothing in your call graph
  can throw.
- **No RTTI required.**
- **A configurable, override-able assertion handler** instead of a hard-coded `abort()`
  — see [Configurable assertions](#configurable-assertions) above.
- **Standard-library-heavy pieces are opt-in headers, not part of the umbrella
  include.** `<chrono>`-based timing (`metrics::Timing`) and `<tuple>`-based `Switch`
  are excluded from `<allocators/allocators.hpp>` by default; include them explicitly if
  you want them. This keeps `#include <allocators/allocators.hpp>` safe to reach for on
  a freestanding target without auditing every transitive header first.

## Design philosophy

A few principles show up repeatedly across the codebase and are worth knowing before you
extend it:

- **Explicit storage provisioning, always.** No allocator silently partitions memory it
  wasn't given; static helpers (`calculate_block_count`, `get_state_size`, ...) let you
  compute exactly how much storage a configuration needs, alignment padding included.
- **Customizable ownership.** When forming a composite allocator, it typically takes ownership of the underlying components, but you can either explicitly pass a reference type as template parameter (we call `forward()` under the hood), or wrap the sub-objects with `composite::Ref` to override this behavior. This forwarding with explicit reference pattern is more of a general guideline and is followed by some things beyond allocator composition, but there are exceptions (mostly for internal types) where taking a reference (which implies potentially sharing the object) is pointless.
- **Policy via concept, not virtual dispatch.** `Block_Allocator` and `State_Concept`
  are structural concepts, not base classes; the assertion system is a compile-time
  configurable macro layer rather than a runtime-polymorphic logger.
- **The block type is the customization axis for memory lifetime.** Want RAII semantics on
  managed memory? Pass `raii_block<>` as the block type template parameter. Want a plain
  non-owning `block`? That's the default. No separate "owning" vs. "borrowing" allocator
  hierarchy.
- **"Why not remove this?"** is the standing question during design iteration. Template
  parameters, abstraction layers, and API surface all have to earn their keep.

## Project status & roadmap

This library is **pre-1.0**. The core architecture (Layout → State → Allocator for
buddy, `Block_Allocator`-concept composition everywhere else) is settled and unlikely to
change; some corners are still rough, and breaking changes are possible before 1.0.

On the horizon, roughly in priority order:

1. **`std::pmr::memory_resource` bridge adapter** — a single-file adapter that opens up
   STL containers to this library; probably the highest-leverage addition for anyone not
   working in a freestanding context.
2. **Allocator traits / static introspection** — optional static members
   (`max_block_size`, `alignment_guarantee`, `is_thread_safe`) that generic code can
   query.
3. **A slab allocator.**
4. **`synchronized<SubAlloc, Mutex>`** — a locking adapter for allocators shared across
   threads.
5. **`verify_invariants()`** — a debug-only consistency checker for the more stateful
   allocators.

Beyond that, the `TODO` file in the repo has a longer wishlist: an affix allocator, a
cascading allocator, logarithmic bucketizers, thread-local/thread-caching/CPU-arena/
queueing allocators for multithreaded use, fragmentation and usage statistics, a hash
specialization for `block`, and a scope-based auto-freeing allocator wrapper.

Known rough edges, for transparency:

- The **utility** allocators (`Switch`, `Owner`, `XRef`) are excluded from the default
  include and are the least stable part of the API.
- Structured error propagation via `std::expected` (`error_handling/result.hpp`) is
  scaffolded but not yet wired up to a concrete error type.

## Comparison & inspiration

This library takes explicit inspiration from
[foonathan/memory](https://github.com/foonathan/memory), Andrei Alexandrescu's
composable-allocator talk at CppCon 2015, and `std::pmr`. Where it differs:

- **Two-value blocks instead of header-based bookkeeping** — no allocation header
  hidden next to your pointer.
- **Concept-checked composition instead of a fixed class hierarchy** — anything
  satisfying `Block_Allocator` composes, with no common base class required.
- **Freestanding/kernel use as a primary target**, not a stretch goal — see
  [Freestanding & kernel support](#freestanding--kernel-support).

## Testing & quality

- Every public header is compiled as its own standalone translation unit
  (`cc_header_compilation_suite` in the Bazel build) to guarantee headers are
  self-sufficient and don't silently depend on include order elsewhere in the project.
- The buddy allocator's block-count-vs-bitmap-size sizing math is checked against
  brute-force and binary-search reference implementations across millions of randomized
  cases (`tests/buddy_allocator_algorithms*.cpp`).
- Internal buddy state — freelist contents, bitmap toggling, splitting and merging — has
  dedicated white-box tests (`tests/buddy_internal_state_test*.cpp`).
- Both the CMake and Bazel builds compile with `-Wall -Wextra -Weffc++ -Wconversion
  -Wsign-conversion -Wshadow` (and more) taken seriously, not just switched on and
  ignored.

## Contributing

Issues and PRs are welcome, especially around:

- The roadmap items above (the `pmr` bridge is the single most useful contribution for
  most users right now).
- Additional allocator types from the `TODO` wishlist.
- Portability testing and fixes on other compilers.

If you're adding a header, please keep it warning-clean under the flags listed in
[Testing & quality](#testing--quality) and add it to the relevant CMake/Bazel test
target so it gets a standalone compilation check for free.

## Support this project

If `dd99::memory` is useful to you, sponsorship helps justify the time spent
maintaining and extending it.

- **GitHub Sponsors:** _coming soon_
- **Bitcoin:** bc1qhc2zg27x8hgw2w9nvg3d7fg5uud6e5z9n4rezp

Also, starring the repo, opening issues, and sharing it with people who'd
find it useful are all genuinely useful forms of support for a project at this stage.

## License

Copyright © 2026 dd99. Licensed under the Apache License, Version 2.0.
