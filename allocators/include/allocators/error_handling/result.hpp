#pragma once

#include <expected>
#include <allocators/error_handling/error_code.hpp>



namespace dd99_allocators_namespace
{

    template <class T>
    using result = std::expected<T, dd99_allocators_namespace::error_code>;


    template <class T>
    using infallible_result = std::expected<T, void>;

}
