#pragma once

#include <allocators/structures/blocks/self_contained_block.hpp>
#include <allocators/block_allocators/basic/bitmap/bitmap.hpp>
#include <allocators/block_allocators/basic/buddy/buddy.hpp>
// #include <allocators/block_allocators/basic/pool.hpp>
// #include <allocators/block_allocators/basic/slicing.hpp>
// #include <allocators/block_allocators/basic/stack.hpp>
#include <allocators/block_allocators/composite/fallback.hpp>
#include <allocators/block_allocators/composite/filter.hpp>
#include <allocators/block_allocators/composite/quantizer.hpp>
#include <allocators/block_allocators/composite/ref.hpp>
#include <allocators/block_allocators/composite/segregator.hpp>
// #include <allocators/block_allocators/composite/throwing.hpp> // disabled (freestanding)
#include <allocators/block_allocators/degenerate/boolean.hpp>
#include <allocators/block_allocators/degenerate/constant.hpp>
#include <allocators/block_allocators/degenerate/null.hpp>
#include <allocators/block_allocators/metrics/stats.hpp>
// #include <allocators/block_allocators/metrics/timing.hpp> // disabled (freestanding)
#include <allocators/pointer_allocators/basic.hpp>
#include <allocators/pointer_allocators/checked.hpp>
#include <allocators/block_allocators/utility/filter.hpp>
#include <allocators/block_allocators/utility/owner.hpp>
// #include <allocators/block_allocators/utility/switch.hpp> // disabled temporarily (requires <tuple>)
#include <allocators/block_allocators/utility/unique_block.hpp>
#include <allocators/exception.hpp>


// NOTE: timing allocator uses <chrono>, which is not part of the current freestanding library implementation.
