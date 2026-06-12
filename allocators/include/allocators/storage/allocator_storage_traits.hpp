#pragma once

#include <allocators/structures/blocks/memory_block.hpp>
#include <tuple>


namespace dd99::memory::storage
{

    namespace detail
    {

        // concept for storage policies that comply with the generic API
        // this can be seen as the definition of the generic storage policy API
        // storage policies that do not comply with this concept can be supported via the customization point below
        template <class P>
        concept Storage_Policy_Concept =
            requires(P & p)
            {
                {p.memory()} -> std::same_as<memory::block>;
                {p.aux_memory()} -> std::same_as<memory::block>;
                {p.block_count()} -> std::same_as<std::size_t>;
            };

    }

    // Customization Point
    template <class Allocator>
    struct allocator_storage_traits;

    // default behavior - just forward the policy
    // TODO: This needs some more thought
    template <class Allocator>
    requires ( requires { typename Allocator::storage_policy; } && dd99::memory::storage::detail::Storage_Policy_Concept<typename Allocator::storage_policy> )
    struct allocator_storage_traits<Allocator>
    {
        using storage_policy = Allocator::storage_policy;

        static constexpr auto resolve_storage(memory::block memory, storage_policy && policy)
        {
            return std::forward_as_tuple<memory::block, storage_policy>(memory, std::forward<storage_policy>(policy));
        }
    };

}
