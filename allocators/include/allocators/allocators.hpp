#pragma once

#include <allocators/basic/bitmap.hpp>
#include <allocators/basic/buddy.hpp>
#include <allocators/basic/pool.hpp>
#include <allocators/basic/slicing.hpp>
#include <allocators/basic/stack.hpp>
#include <allocators/borrowing/bitmap.hpp>
#include <allocators/borrowing/buddy.hpp>
#include <allocators/composite/fallback.hpp>
#include <allocators/composite/filter.hpp>
#include <allocators/composite/quantizer.hpp>
#include <allocators/composite/ref.hpp>
#include <allocators/composite/segregator.hpp>
#include <allocators/composite/throwing.hpp>
#include <allocators/degenerate/boolean.hpp>
#include <allocators/degenerate/constant.hpp>
#include <allocators/degenerate/null.hpp>
#include <allocators/internal/bases/buddy.hpp>
#include <allocators/metrics/stats.hpp>
#include <allocators/metrics/timing.hpp>
#include <allocators/pointer/basic.hpp>
#include <allocators/pointer/checked.hpp>
#include <allocators/utility/filter.hpp>
#include <allocators/utility/owner.hpp>
#include <allocators/utility/switch.hpp>
#include <allocators/utility/unique_block.hpp>

// #include <allocators/multiblock
