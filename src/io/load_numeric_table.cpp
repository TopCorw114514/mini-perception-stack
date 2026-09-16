#include "sensekit/io/load_numeric_table.hpp"

#include <charconv>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace sensekit::io {
namespace {

constexpr std::string_view kUtf8Bom = "\xEF\xBB\xBF";
constexpr std::string_view kWhitespace = " \t\r\n\f\v";

[[nodiscard]] std::string_view trim(std::string_view text) {
    const auto first = text.find_first_not_of(kWhitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(kWhitespace);
    return text.substr(first, last - first + 1);
}

/// Splits one content line into raw fields. Comma mode keeps empty fields so
/// that `1,,3` can be reported as an error instead of silently shifting columns.
[[nodiscard]] std::vector<std::string_view> split_line(std::string_view line,
                                                       Delimiter delimiter) {
    std::vector<std::string_view> fields;

    if (delimiter == Delimiter::Comma) {
        std::size_t begin = 0;
        for (;;) {
            const auto comma = line.find(',', begin);
            if (comma == std::string_view::npos) {
                fields.push_back(line.substr(begin));
                break;
            }
            fields.push_back(line.substr(begin, comma - begin));
            begin = comma + 1;
        }
        return fields;
    }

    std::size_t position = 0;
    while (position < line.size()) {
        while (position < line.size() && (line[position] == ' ' || line[position] == '\t')) {
            ++position;
        }
        if (position >= line.size()) {
            break;
        }
        const auto begin = position;
        while (position < line.size() && line[position] != ' ' && line[position] != '\t') {
            ++position;
        }
        fields.push_back(line.substr(begin, position - begin));
    }
    return fields;
}

struct ParsedField {
    double value = 0.0;
    bool is_number = false;
};

[[nodiscard]] ParsedField parse_field(std::string_view field) {
    const auto text = trim(field);
    if (text.empty()) {
        return {};
    }
    double value = 0.0;
    const auto result =
        std::from_chars(text.data(), text.data() + text.size(), value, std::chars_format::general);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        return {};
    }
    return {value, true};
}

[[nodiscard]] NumericTable parse_impl(std::string_view text,
                                      const LoadOptions& options,
                                      std::string source_name) {
    Delimiter delimiter = options.delimiter;
    bool delimiter_known = delimiter != Delimiter::Auto;
    bool header_pending = options.header != HeaderMode::None;
    const bool auto_detect_header = options.header == HeaderMode::Auto;

    std::vector<std::string> column_names;
    std::vector<std::vector<double>> rows;
    std::size_t column_count = 0;

    const auto handle_content_line = [&](std::string_view content, std::size_t line_number) {
        if (!delimiter_known) {
            delimiter = content.find(',') != std::string_view::npos ? Delimiter::Comma
                                                                   : Delimiter::Whitespace;
            delimiter_known = true;
        }

        const auto fields = split_line(content, delimiter);

        std::vector<double> values;
        values.reserve(fields.size());
        std::size_t first_bad_field = 0;
        bool all_numbers = true;
        for (std::size_t index = 0; index < fields.size(); ++index) {
            const auto parsed = parse_field(fields[index]);
            if (!parsed.is_number) {
                all_numbers = false;
                first_bad_field = index;
                break;
            }
            values.push_back(parsed.value);
        }

        if (header_pending) {
            header_pending = false;
            const bool treat_as_header = !auto_detect_header || !all_numbers;
            if (treat_as_header) {
                column_names.reserve(fields.size());
                for (const auto field : fields) {
                    column_names.emplace_back(trim(field));
                }
                return;
            }
        }

        if (!all_numbers) {
            throw LoadError(LoadErrorKind::NonNumericToken,
                            source_name + ":" + std::to_string(line_number) + ": column " +
                                std::to_string(first_bad_field + 1) +
                                " is not a number: '" +
                                std::string(trim(fields[first_bad_field])) + "'",
                            line_number);
        }

        if (column_count == 0) {
            column_count = values.size();
        } else if (values.size() != column_count) {
            throw LoadError(LoadErrorKind::MalformedRow,
                            source_name + ":" + std::to_string(line_number) + ": expected " +
                                std::to_string(column_count) + " columns but found " +
                                std::to_string(values.size()),
                            line_number);
        }

        if (options.reject_non_finite) {
            for (std::size_t index = 0; index < values.size(); ++index) {
                if (!std::isfinite(values[index])) {
                    throw LoadError(LoadErrorKind::NonFiniteValue,
                                    source_name + ":" + std::to_string(line_number) + ": column " +
                                        std::to_string(index + 1) +
                                        " is not a finite number (strict mode)",
                                    line_number);
                }
            }
        }

        rows.push_back(std::move(values));
    };

    std::size_t line_begin = 0;
    std::size_t line_number = 0;
    while (line_begin <= text.size()) {
        std::size_t line_end = text.find('\n', line_begin);
        const bool last_line = line_end == std::string_view::npos;
        if (last_line) {
            line_end = text.size();
        }
        ++line_number;

        std::string_view line = text.substr(line_begin, line_end - line_begin);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }
        if (line_number == 1 && line.starts_with(kUtf8Bom)) {
            line.remove_prefix(kUtf8Bom.size());
        }

        const auto content = trim(line);
        if (!content.empty() && content.front() != '#') {
            handle_content_line(content, line_number);
        }

        if (last_line) {
            break;
        }
        line_begin = line_end + 1;
    }

    if (rows.empty()) {
        throw LoadError(LoadErrorKind::EmptyInput, source_name + ": no data rows found");
    }

    if (column_names.empty()) {
        column_names.reserve(column_count);
        for (std::size_t index = 0; index < column_count; ++index) {
            column_names.push_back("column_" + std::to_string(index));
        }
    } else if (column_names.size() != column_count) {
        throw LoadError(LoadErrorKind::MalformedRow,
                        source_name + ": header has " + std::to_string(column_names.size()) +
                            " columns but data rows have " + std::to_string(column_count));
    }

    return NumericTable(std::move(column_names), std::move(rows), std::move(source_name));
}

/// Those bytes are UTF-8. `std::filesystem::path::string()` would convert them
/// to the active code page on Windows instead, which turns a Chinese folder name
/// into mojibake in every diagnostic this library prints.
[[nodiscard]] std::string to_utf8_text(const std::filesystem::path& path) {
    const auto text = path.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

}  // namespace

NumericTable parse_numeric_table(const std::string& text,
                                 const LoadOptions& options,
                                 std::string source_name) {
    return parse_impl(text, options, std::move(source_name));
}

NumericTable load_numeric_table(const std::filesystem::path& path, const LoadOptions& options) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw LoadError(LoadErrorKind::FileAccess,
                        "cannot open input file '" + to_utf8_text(path) + "'");
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    if (file.bad()) {
        throw LoadError(LoadErrorKind::FileAccess,
                        "cannot read input file '" + to_utf8_text(path) + "'");
    }

    const std::string content = buffer.str();
    return parse_impl(content, options, to_utf8_text(path));
}

}  // namespace sensekit::io
