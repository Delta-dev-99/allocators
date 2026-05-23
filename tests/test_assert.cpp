
// #include <assert.h>
#include <allocators/library_configuration/assertions.hpp>
#include <iostream>
#include <string_view>
// #include <stacktrace>

#include <source_location>


inline constexpr void my_assert(bool exp, std::string_view file = __builtin_FILE(), const int line = __builtin_LINE(), std::string_view func = __builtin_FUNCTION())
{
    
    std::cout << "assert called on '" << file << "':" << line << " on function: " << func << " \n";
}


void f(int x)
{
    my_assert(x > 5);
}


int main()
{
    DD99_ALLOCATOR_ASSERT(true);
    DD99_ALLOCATOR_ASSERT(false);

    my_assert(("some bad expression", 0));

    f(3);

    std::cout << __builtin_FILE() << "\n";
    std::cout << __builtin_LINE() << "\n";
}
