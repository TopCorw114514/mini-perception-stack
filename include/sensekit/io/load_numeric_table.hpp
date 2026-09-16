#pragma once

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "sensekit/io/numeric_table.hpp"

namespace sensekit::io {

enum class Delimiter {
    /// Comma when the first content line contains a comma, whitespace otherwise.
    Auto,
    Comma,
    /// One or more spaces or tabs between fields.
    Whitespace,
};

enum class HeaderMode {
    /// Treat the first content line as a header when it contains a non-number.
    Auto,
    None,
    FirstRow,
};

struct LoadOptions {
    Delimiter delimiter = Delimiter::Auto;
    HeaderMode header = HeaderMode::Auto;
    /// Reject NaN and infinity instead of storing them (`--strict` in the CLI).
    bool reject_non_finite = false;
};

enum class LoadErrorKind {
    /// The file could not be opened or read.
    FileAccess,
    /// The file contains no data row at all.
    EmptyInput,
    /// A row has a different number of columns than the first row.
    MalformedRow,
    /// A field is not a number.
    NonNumericToken,
    /// A field is NaN or infinity while `reject_non_finite` is set.
    NonFiniteValue,
};

class LoadError : public std::runtime_error {
public:
    LoadError(LoadErrorKind kind, const std::string& message, std::size_t line_number = 0)
        : std::runtime_error(message), kind_(kind), line_number_(line_number) {}

    [[nodiscard]] LoadErrorKind kind() const noexcept { return kind_; }

    /// 1-based physical line number in the source text, 0 when not applicable.
    [[nodiscard]] std::size_t line_number() const noexcept { return line_number_; }

private:
    LoadErrorKind kind_;
    std::size_t line_number_;
};

/// Reads a numeric table from a text file.
///
/// Rules that apply to every mode:
///   * a UTF-8 BOM on the first line is ignored;
///   * LF and CRLF line endings both work;
///   * blank lines are skipped;
///   * a line whose first non-blank character is `#` is a comment and skipped;
///   * every data row must have exactly as many fields as the first data row;
///   * fields are trimmed, so ` 1 , 2 ` is two fields with or without a comma.
[[nodiscard]] NumericTable load_numeric_table(const std::filesystem::path& path,
                                              const LoadOptions& options = {});

/// Same parsing rules, but reads from an in-memory blob. Used by tests and by
/// the CLI when it needs to report the logical source name in errors.
[[nodiscard]] NumericTable parse_numeric_table(const std::string& text,
                                               const LoadOptions& options = {},
                                               std::string source_name = "<memory>");

}  // namespace sensekit::io
