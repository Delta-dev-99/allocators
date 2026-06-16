#include <allocators/library_configuration/assertions.hpp>
#include <iostream>
#include <cstdlib>
#include <format>



namespace dd99::memory
{
    [[weak, cold, noreturn]] // TODO: write appropriate attributes
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