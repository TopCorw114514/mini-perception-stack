#include <iostream>
#include <string>
#include <vector>

#include "sensekit/cli/stats_cli.hpp"

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
