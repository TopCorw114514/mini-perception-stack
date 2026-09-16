#include "sensekit/stats/stats.hpp"

#include <cmath>
#include <stdexcept>

namespace sensekit::stats {
namespace {

[[nodiscard]] constexpr int ddof_divisor(Ddof ddof) noexcept {
    return ddof == Ddof::Sample ? 1 : 0;
}

void require_not_empty(std::span<const double> values) {
    if (values.empty()) {
        throw std::invalid_argument("sensekit::stats: at least one value is required");
    }
}

}  // namespace

double mean(std::span<const double> values) {
    require_not_empty(values);
    double sum = 0.0;
    for (const auto value : values) {
        sum += value;
    }
    return sum / static_cast<double>(values.size());
}

double variance(std::span<const double> values, Ddof ddof) {
    require_not_empty(values);

    const auto divisor = static_cast<double>(values.size()) - static_cast<double>(ddof_divisor(ddof));
    if (divisor <= 0.0) {
        throw std::invalid_argument(
            "sensekit::stats: sample variance needs at least two values");
    }

    const auto average = mean(values);
    double sum_of_squares = 0.0;
    for (const auto value : values) {
        const auto deviation = value - average;
        sum_of_squares += deviation * deviation;
    }
    return sum_of_squares / divisor;
}

double rms(std::span<const double> values) {
    require_not_empty(values);
    double sum_of_squares = 0.0;
    for (const auto value : values) {
        sum_of_squares += value * value;
    }
    return std::sqrt(sum_of_squares / static_cast<double>(values.size()));
}

ColumnStats compute_column_stats(std::span<const double> values, Ddof ddof) {
    require_not_empty(values);
    ColumnStats summary;
    summary.count = values.size();
    summary.mean = mean(values);
    summary.variance = variance(values, ddof);
    summary.rms = rms(values);
    return summary;
}

}  // namespace sensekit::stats
