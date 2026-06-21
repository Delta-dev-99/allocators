#include <allocators/library_configuration/assertions.hpp>
#include <iostream>
#include <cstdlib>
#include <format>



namespace dd99::memory
{
    // NOTE: this is provided as a weak symbol.
    // the user may want to provide a custom implementation.
    // this won't compile on some environments (e.g. freestanding)
    //  in that case, don't use this file and just provide an appropriate implementation or a stub.
    //  for a kernel it may be reasonable to replace with a call to `panic()` or an equivalent.
    [[gnu::weak, gnu::cold, noreturn]]
    void
    allocators_assertion_failed(const assertion_info & info) noexcept
    {
        auto level_str = [](unsigned level){
            switch (level)
            {
            case 1: return std::string_view{"critical"};
            case 2: return std::string_view{"hardened"};
            case 3: return std::string_view{"debug"};
            default: return std::string_view{"unknown"};
            }
        }(info.level);

        std::cout << std::format("{}:{}: {} assertion failed. {}. expression: {}", info.file, info.line, level_str, info.message, info.expression);
        std::cout << std::endl;
        std::abort();
    }
}