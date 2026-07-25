#pragma once

#include <allocators/block_allocators/block_allocator.hpp>

#include <utility>
#include <tuple>
#include <type_traits>


namespace dd99_allocators_namespace::block_allocator::utility
{

    // Requirements:
    //  - Sub allocators are "Block Allocators"
    //  - Functor can be evaluated on requests
    //  - Functor returns an index into the list of allocators
    //  - Request type must provide member function `get_request()`
    //      used to get request to pass down to sub allocators
    // Template args:
    //  - AV a parameter pack where each parameter is a pair of an Allocator type
    //    and a compile-time constant value
    // Ussage:
    // Switch my_switch(func, alloc1, alloc2, ...)
    // 
    // TODO: This needs restructuring because it is hard to read, and because now there are better ways.
    template <class Functor, class... Allocators>
    class Switch
    {
        using count_type = decltype(sizeof...(Allocators));
        static constexpr count_type allocator_count = sizeof...(Allocators);
        using index_sequence = std::make_index_sequence<allocator_count>;

    public:
        Switch(Functor functor, Allocators ... allocators)
            : m_allocators(std::forward_as_tuple<Allocators...>(allocators...))
            , m_functor(std::move(functor))
        { }

        Switch(const Switch&) = delete;
        Switch(Switch&&) = default;
        Switch & operator=(const Switch &) = delete;
        Switch & operator=(Switch &&) = delete;

    private: // implementation details
        template <class Request, std::size_t... Indices>
        block allocate(Request request, std::index_sequence<Indices...>)
        {
            using ret_type = std::common_type_t<
                decltype(std::get<Indices>(m_allocators).allocate(request.get_request()))...>;

            const auto i = m_functor(request);
            ret_type ret{};
            static_cast<void>(std::initializer_list<std::size_t>
                { (i == Indices ?
                    (ret = std::get<Indices>(m_allocators).allocate(
                        request.get_request())), 0U : 0U)... });
            return ret;
        }

        template <std::size_t... Indices>
        void deallocate(const block & memory, std::index_sequence<Indices...>)
        {
            bool k = (... || (std::get<Indices>(m_allocators).owns(memory)
                ? (std::get<Indices>(m_allocators).deallocate(memory), true)
                : false));
            static_cast<void>(k);
        }

        template <std::size_t... Indices>
        void deallocate_all(std::index_sequence<Indices...>)
        {
            static_cast<void>(std::initializer_list<std::size_t>
                { (std::get<Indices>(m_allocators).deallocate_all(), 0U)... });
        }

        template <class T, std::size_t... Indices>
        bool owns(T mem, std::index_sequence<Indices...>) const
        {
            return (... || std::get<Indices>(m_allocators).owns(mem));
        }

    public:
        template <class Request>
        [[nodiscard]]
        block allocate(Request request)
        {
            return allocate(request, index_sequence{});
        }

        void deallocate(const block &memory)
        {
            return deallocate(memory, index_sequence{});
        }

        void deallocate_all()
        {
            return deallocate_all(index_sequence{});
        }

        bool owns(const std::byte * memory) const
        {
            return owns(memory, index_sequence{});
        }

        bool owns(const block &memory) const
        {
            return owns(memory, index_sequence{});
        }

    public:
        std::tuple<Allocators...> m_allocators;

    private:
        Functor m_functor;
    };

}



// Switch will accept custom request type
// and relay the request to the apropiate allocator
// based on the result of a function on the request.
// Switch will not be an "Allocator"
