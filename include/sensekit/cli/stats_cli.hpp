#pragma once

#include <iosfwd>
#include <span>
#include <string>

namespace sensekit::cli {

/// Exit codes of the `sensekit-stats` command line tool.
enum class ExitCode : int {
    kSuccess = 0,
    /// Bad arguments, unreadable input file, unwritable output file.
    kUsageOrIoError = 2,
    /// The input file was readable but did not contain a valid numeric table.
    kMalformedData = 3,
};

/// Runs `sensekit-stats`. `args` excludes the program name, exactly like
/// `argv[1..]`, and every argument is UTF-8 encoded: apps/sensekit_stats/main.cpp
/// converts the Windows UTF-16 command line before calling this. Results go to
/// `out`, diagnostics to `err`.
///
/// Keeping this as a library function instead of a `main()` full of logic is
/// what makes the end-to-end tests in tests/test_stats_cli.cpp possible without
/// spawning a child process.
[[nodiscard]] int run_stats_cli(std::span<const std::string> args,
                                std::ostream& out,
                                std::ostream& err);

/// The text printed by `--help`.
[[nodiscard]] std::string stats_cli_usage();

}  // namespace sensekit::cli
