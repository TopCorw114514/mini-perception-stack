#include "sensekit/cli/stats_cli.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "sensekit/io/load_numeric_table.hpp"
#include "sensekit/stats/stats.hpp"

namespace sensekit::cli {
namespace {

constexpr std::string_view kWhitespace = " \t\r\n\f\v";

[[nodiscard]] constexpr int to_exit_code(ExitCode code) noexcept {
    return static_cast<int>(code);
}

/// Thrown for anything the user can fix on the command line.
struct UsageError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Options {
    std::filesystem::path input;
    std::filesystem::path output;
    io::LoadOptions load;
    stats::Ddof ddof = stats::Ddof::Population;
    std::string columns_spec = "all";
    bool show_help = false;
};

[[nodiscard]] std::string_view trim(std::string_view text) {
    const auto first = text.find_first_not_of(kWhitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(kWhitespace);
    return text.substr(first, last - first + 1);
}

[[nodiscard]] std::vector<std::string_view> split_commas(std::string_view text) {
    std::vector<std::string_view> parts;
    std::size_t begin = 0;
    for (;;) {
        const auto comma = text.find(',', begin);
        if (comma == std::string_view::npos) {
            parts.push_back(text.substr(begin));
            break;
        }
        parts.push_back(text.substr(begin, comma - begin));
        begin = comma + 1;
    }
    return parts;
}

[[nodiscard]] bool is_index(std::string_view text) {
    return !text.empty() &&
           std::all_of(text.begin(), text.end(),
                       [](unsigned char character) { return std::isdigit(character) != 0; });
}

/// Wraps a field in quotes only when the CSV dialect requires it.
[[nodiscard]] std::string to_csv_field(std::string_view text) {
    if (text.find_first_of(",\"\r\n") == std::string_view::npos) {
        return std::string(text);
    }
    std::string result;
    result.reserve(text.size() + 2);
    result.push_back('"');
    for (const auto character : text) {
        if (character == '"') {
            result.push_back('"');
        }
        result.push_back(character);
    }
    result.push_back('"');
    return result;
}

[[nodiscard]] stats::Ddof parse_ddof(std::string_view text) {
    if (text == "0") {
        return stats::Ddof::Population;
    }
    if (text == "1") {
        return stats::Ddof::Sample;
    }
    throw UsageError("--ddof expects 0 or 1, got '" + std::string(text) + "'");
}

[[nodiscard]] io::Delimiter parse_delimiter(std::string_view text) {
    if (text == "auto") {
        return io::Delimiter::Auto;
    }
    if (text == "comma") {
        return io::Delimiter::Comma;
    }
    if (text == "whitespace") {
        return io::Delimiter::Whitespace;
    }
    throw UsageError("--delimiter expects auto, comma or whitespace, got '" + std::string(text) +
                     "'");
}

[[nodiscard]] io::HeaderMode parse_header_mode(std::string_view text) {
    if (text == "auto") {
        return io::HeaderMode::Auto;
    }
    if (text == "none") {
        return io::HeaderMode::None;
    }
    if (text == "first-row") {
        return io::HeaderMode::FirstRow;
    }
    throw UsageError("--header expects auto, none or first-row, got '" + std::string(text) + "'");
}

void parse_options(std::span<const std::string> args, Options& options) {
    bool input_seen = false;

    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& argument = args[index];

        // Support both `--flag value` and `--flag=value`.
        std::string_view name = argument;
        std::string_view inline_value;
        bool has_inline_value = false;
        if (const auto equals = argument.find('='); equals != std::string::npos) {
            name = std::string_view(argument).substr(0, equals);
            inline_value = std::string_view(argument).substr(equals + 1);
            has_inline_value = true;
        }

        const auto take_value = [&]() -> std::string_view {
            if (has_inline_value) {
                return inline_value;
            }
            if (index + 1 >= args.size()) {
                throw UsageError("missing value for " + std::string(name));
            }
            return args[++index];
        };

        if (name == "-h" || name == "--help") {
            options.show_help = true;
            return;
        }
        if (name == "--strict") {
            options.load.reject_non_finite = true;
            continue;
        }
        if (name == "--delimiter") {
            options.load.delimiter = parse_delimiter(take_value());
            continue;
        }
        if (name == "--header") {
            options.load.header = parse_header_mode(take_value());
            continue;
        }
        if (name == "--ddof") {
            options.ddof = parse_ddof(take_value());
            continue;
        }
        if (name == "--columns") {
            options.columns_spec = std::string(take_value());
            continue;
        }
        if (name == "--output") {
            options.output = std::string(take_value());
            continue;
        }
        if (!argument.empty() && argument.front() == '-') {
            throw UsageError("unknown option '" + argument + "'");
        }
        if (input_seen) {
            throw UsageError("unexpected extra argument '" + argument + "'");
        }
        options.input = argument;
        input_seen = true;
    }
}

[[nodiscard]] std::vector<std::size_t> resolve_columns(const io::NumericTable& table,
                                                       std::string_view spec) {
    std::vector<std::size_t> columns;

    if (spec.empty() || spec == "all") {
        columns.resize(table.column_count());
        for (std::size_t index = 0; index < columns.size(); ++index) {
            columns[index] = index;
        }
        return columns;
    }

    for (const auto raw : split_commas(spec)) {
        const auto token = trim(raw);
        if (token.empty()) {
            throw UsageError("--columns contains an empty entry");
        }

        if (is_index(token)) {
            std::size_t index = 0;
            const auto result =
                std::from_chars(token.data(), token.data() + token.size(), index);
            if (result.ec != std::errc{} || index >= table.column_count()) {
                throw UsageError("--columns index " + std::string(token) +
                                 " is out of range (0.." +
                                 std::to_string(table.column_count() - 1) + ")");
            }
            columns.push_back(index);
            continue;
        }

        const auto index = table.find_column(std::string(token));
        if (index == io::NumericTable::npos) {
            throw UsageError("--columns name '" + std::string(token) +
                             "' is not a column of this table");
        }
        columns.push_back(index);
    }
    return columns;
}

}  // namespace

std::string stats_cli_usage() {
    return
        "sensekit-stats - per column statistics for numeric text tables\n"
        "\n"
        "usage:\n"
        "  sensekit-stats <file> [options]\n"
        "\n"
        "options:\n"
        "  --delimiter auto|comma|whitespace    field separator (default: auto)\n"
        "  --header auto|none|first-row         header handling (default: auto)\n"
        "  --columns all|0,2|body_acc_x,...     0-based indices or column names\n"
        "                                       (default: all)\n"
        "  --ddof 0|1                           variance divisor N or N-1 (default: 0)\n"
        "  --output <file>                      write the CSV to a file instead of stdout\n"
        "  --strict                             treat NaN and infinity as errors\n"
        "  -h, --help                           show this help\n"
        "\n"
        "output columns: column,count,mean,variance,rms\n"
        "\n"
        "examples:\n"
        "  sensekit-stats body_acc_x_train.txt\n"
        "  sensekit-stats signals.csv --header first-row --columns mean,rms --ddof 1\n"
        "\n"
        "exit codes: 0 ok, 2 usage or file error, 3 malformed data\n";
}

int run_stats_cli(std::span<const std::string> args, std::ostream& out, std::ostream& err) {
    Options options;
    try {
        parse_options(args, options);
    } catch (const UsageError& error) {
        err << "sensekit-stats: " << error.what() << "\n\n" << stats_cli_usage();
        return to_exit_code(ExitCode::kUsageOrIoError);
    }

    if (options.show_help) {
        out << stats_cli_usage();
        return to_exit_code(ExitCode::kSuccess);
    }

    if (options.input.empty()) {
        err << "sensekit-stats: no input file given\n\n" << stats_cli_usage();
        return to_exit_code(ExitCode::kUsageOrIoError);
    }

    io::NumericTable table;
    try {
        table = io::load_numeric_table(options.input, options.load);
    } catch (const io::LoadError& error) {
        err << "sensekit-stats: " << error.what() << '\n';
        return to_exit_code(error.kind() == io::LoadErrorKind::FileAccess
                                ? ExitCode::kUsageOrIoError
                                : ExitCode::kMalformedData);
    }

    std::vector<std::size_t> columns;
    try {
        columns = resolve_columns(table, options.columns_spec);
    } catch (const UsageError& error) {
        err << "sensekit-stats: " << error.what() << '\n';
        return to_exit_code(ExitCode::kUsageOrIoError);
    }

    std::ostringstream buffer;
    buffer << "column,count,mean,variance,rms\n";
    // %.10g style: short enough to read, precise enough to compare with numpy.
    buffer << std::setprecision(10);
    try {
        for (const auto index : columns) {
            const auto values = table.column(index);
            const auto summary = stats::compute_column_stats(values, options.ddof);
            buffer << to_csv_field(table.column_name(index)) << ',' << summary.count << ','
                   << summary.mean << ',' << summary.variance << ',' << summary.rms << '\n';
        }
    } catch (const std::invalid_argument& error) {
        err << "sensekit-stats: " << error.what() << '\n';
        return to_exit_code(ExitCode::kMalformedData);
    }

    const auto payload = buffer.str();
    if (options.output.empty()) {
        out << payload;
        return to_exit_code(ExitCode::kSuccess);
    }

    std::ofstream file(options.output, std::ios::binary | std::ios::trunc);
    if (!file) {
        err << "sensekit-stats: cannot open output file '" << options.output.string() << "'\n";
        return to_exit_code(ExitCode::kUsageOrIoError);
    }
    file << payload;
    file.flush();
    if (!file) {
        err << "sensekit-stats: cannot write output file '" << options.output.string() << "'\n";
        return to_exit_code(ExitCode::kUsageOrIoError);
    }
    return to_exit_code(ExitCode::kSuccess);
}

}  // namespace sensekit::cli
