#include "sensekit/io/numeric_table.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace sensekit::io {
namespace {

[[nodiscard]] std::string out_of_range_message(std::string_view what,
                                               std::size_t index,
                                               std::size_t count) {
    return "NumericTable::" + std::string(what) + ": index " + std::to_string(index) +
           " is out of range (size is " + std::to_string(count) + ")";
}

}  // namespace

const std::string& NumericTable::column_name(std::size_t index) const {
    if (index >= column_names_.size()) {
        throw std::out_of_range(out_of_range_message("column_name", index, column_names_.size()));
    }
    return column_names_[index];
}

const std::vector<double>& NumericTable::row(std::size_t index) const {
    if (index >= rows_.size()) {
        throw std::out_of_range(out_of_range_message("row", index, rows_.size()));
    }
    return rows_[index];
}

std::vector<double> NumericTable::column(std::size_t index) const {
    if (index >= column_names_.size()) {
        throw std::out_of_range(out_of_range_message("column", index, column_names_.size()));
    }
    std::vector<double> values;
    values.reserve(rows_.size());
    for (const auto& current : rows_) {
        values.push_back(current[index]);
    }
    return values;
}

bool NumericTable::has_column(const std::string& name) const noexcept {
    return find_column(name) != npos;
}

std::size_t NumericTable::find_column(const std::string& name) const noexcept {
    for (std::size_t index = 0; index < column_names_.size(); ++index) {
        if (column_names_[index] == name) {
            return index;
        }
    }
    return npos;
}

}  // namespace sensekit::io
