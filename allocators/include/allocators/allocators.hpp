
// First two files currently don't work
// #include <allocators/basic/bitmap.hpp>
// #include <allocators/basic/buddy.hpp>
#include <allocators/basic/pool.hpp>
#include <allocators/basic/slicing.hpp>
#include <allocators/basic/stack.hpp>

#include <allocators/utility/ref.hpp>

#include <allocators/borrowing/bitmap.hpp>
#include <allocators/borrowing/buddy.hpp>

#include <allocators/composite/bucketizer.hpp>
#include <allocators/composite/fallback.hpp>
#include <allocators/composite/segregator.hpp>
#include <allocators/composite/throwing.hpp>

#include <allocators/degenerate/boolean.hpp>
#include <allocators/degenerate/constant.hpp>
#include <allocators/degenerate/failed.hpp>

#include <allocators/metrics/stats.hpp>
#include <allocators/metrics/timing.hpp>

// #include <allocators/multiblock

#include <allocators/pointer/basic.hpp>
#include <allocators/pointer/checked.hpp>
