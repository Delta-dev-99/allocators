#pragma once

#include <allocators/storage/allocator_storage_traits.hpp>
#include <allocators/error_handling/result.hpp>



namespace dd99::memory::storage
{

    template <class SubAlloc>
    struct storage_policy_sub_allocator
    {
        using sub_allocator_type = SubAlloc;

        sub_allocator_type m_sub_allocator;
    };


    namespace detail
    {
        // detect specializations of the storage_policy_sub_allocator class template
        template <class>
        struct is_storage_policy_sub_allocator : std::false_type {};

        template <class Arg>
        struct is_storage_policy_sub_allocator<storage_policy_sub_allocator<Arg>> : std::true_type {};

        // a concept for ease of use in constraints
        template <class T>
        concept Storage_Policy_Sub_Allocator_Concept = is_storage_policy_sub_allocator<T>::value;


        // detect allocators which use an instantiation of the storage_policy_sub_allocator class template as policy type
        template <class T>
        concept Allocator_With_Storage_Policy_Sub_Allocator =
            requires { typename T::storage_policy; } && Storage_Policy_Sub_Allocator_Concept<typename T::storage_policy>;
    }


    // Customization Point
    // specialization for the sub-allocator storage policy
    template <dd99::memory::storage::detail::Allocator_With_Storage_Policy_Sub_Allocator Allocator>
    struct allocator_storage_traits
    {
        using storage_policy_type = Allocator::storage_policy;

        struct storage_type
        {
            storage_policy::sub_allocator_type m_sub_allocator;
            memory::block m_aux_memory;
            memory::block m_memory;
        };

        // fallible operation
        static constexpr
        dd99::memory::result<storage_type>
        resolve_storage(memory::block memory, storage_policy_type && policy)
        {
            auto aux_memory = policy.m_sub_allocator.allocate();
            if (aux_memory) // aux_memory.base != nullptr
            {
                return storage_type{
                    std::move(policy.m_sub_allocator),
                    aux_memory,
                    memory
                };
            }
            else
            {
                // TODO: return error result
            }
        }
    };

}
