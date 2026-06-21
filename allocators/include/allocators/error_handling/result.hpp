#pragma once

#include <expected>
#include <allocators/error_handling/error_code.hpp>



namespace dd99::memory
{

    template <class T>
    using result = std::expected<T, dd99::memory::error_code>;


    template <class T>
    using infallible_result = std::expected<T, void>;

}
