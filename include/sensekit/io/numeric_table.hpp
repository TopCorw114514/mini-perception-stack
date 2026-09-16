#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace sensekit::io {

/// A dense table of doubles with one label per column.
///
/// Rows are stored contiguously so that a whole row can be handed to feature
/// code later on. `column()` currently returns a copy, which is fine for the
/// small tables of stage 1. Stage 2 will add a non-owning column view so that
/// sliding windows can reuse one buffer instead of copying the table.
class NumericTable {
public:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    NumericTable() = default;

    NumericTable(std::vector<std::string> column_names,
                 std::vector<std::vector<double>> rows,
                 std::string source_path = {})
        : column_names_(std::move(column_names)),
          rows_(std::move(rows)),
          source_path_(std::move(source_path)) {}

    [[nodiscard]] std::size_t row_count() const noexcept { return rows_.size(); }
    [[nodiscard]] std::size_t column_count() const noexcept { return column_names_.size(); }
    [[nodiscard]] bool empty() const noexcept { return rows_.empty(); }

    [[nodiscard]] const std::vector<std::string>& column_names() const noexcept { return column_names_; }
    [[nodiscard]] const std::vector<std::vector<double>>& rows() const noexcept { return rows_; }

    /// 1-based position in the file for diagnostics, empty when unknown.
    [[nodiscard]] const std::string& source_path() const noexcept { return source_path_; }
    void set_source_path(std::string path) { source_path_ = std::move(path); }

    /// Throws std::out_of_range when `index` is not a valid column.
    [[nodiscard]] const std::string& column_name(std::size_t index) const;

    /// Throws std::out_of_range when `index` is not a valid row.
    [[nodiscard]] const std::vector<double>& row(std::size_t index) const;

    /// Copy of one column. Documented as a copy on purpose: stage 2 replaces
    /// this with a strided view once sliding windows need zero-copy access.
    [[nodiscard]] std::vector<double> column(std::size_t index) const;

    [[nodiscard]] bool has_column(const std::string& name) const noexcept;

    /// 0-based index of `name`, or `npos` when the table has no such column.
    [[nodiscard]] std::size_t find_column(const std::string& name) const noexcept;

private:
    std::vector<std::string> column_names_;
    std::vector<std::vector<double>> rows_;
    std::string source_path_;
};

}  // namespace sensekit::io
