#pragma once

// AI generated

#include <allocators/structures/blocks/memory_block.hpp>
#include <format>

namespace dd99::memory
{
    // (Optional) You could also add a simple formatter for std::byte*
    // if needed elsewhere, but here we directly format inside the block formatter.
}

template <>
class std::formatter<dd99::memory::block>
{
public:
    constexpr auto parse(std::format_parse_context& ctx)
    {
        // No format specifiers for now; just consume everything until '}'.
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}')
            throw std::format_error("invalid format args for dd99::memory::block");
        return it;
    }

    template <class FormatContext>
    auto format(const dd99::memory::block& blk, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(),
                              "{{base={}, size={}}}",
                              static_cast<const void*>(blk.base),
                              blk.size);
    }
};
