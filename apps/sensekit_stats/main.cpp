#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "sensekit/cli/stats_cli.hpp"

#if defined(_WIN32)

#include <windows.h>

namespace {

/// Windows hands a console program a UTF-16 command line while the rest of this
/// program speaks UTF-8, so the conversion happens exactly once, right here.
[[nodiscard]] std::string to_utf8(const wchar_t* text) {
    if (text == nullptr || text[0] == L'\0') {
        return {};
    }
    const int length =
        ::WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1) {
        return {};
    }
    std::string result(static_cast<std::size_t>(length), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), length, nullptr, nullptr);
    result.resize(static_cast<std::size_t>(length) - 1);
    return result;
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    // Make the console interpret the UTF-8 bytes printed below.
    ::SetConsoleOutputCP(CP_UTF8);

    std::vector<std::string> args;
    if (argc > 1) {
        args.reserve(static_cast<std::size_t>(argc - 1));
        for (int index = 1; index < argc; ++index) {
            args.push_back(to_utf8(argv[index]));
        }
    }
    return sensekit::cli::run_stats_cli(args, std::cout, std::cerr);
}

#else

int main(int argc, char** argv) {
    std::vector<std::string> args;
    if (argc > 1) {
        args.reserve(static_cast<std::size_t>(argc - 1));
        for (int index = 1; index < argc; ++index) {
            args.emplace_back(argv[index]);
        }
    }
    return sensekit::cli::run_stats_cli(args, std::cout, std::cerr);
}

#endif
