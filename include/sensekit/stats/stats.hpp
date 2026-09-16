#pragma once

#include <cstddef>
#include <span>

namespace sensekit::stats {

/// Divisor used by `variance`.
enum class Ddof {
    /// Divide by N. This matches numpy's `ddof=0` and is the default.
    Population,
    /// Divide by N-1 (Bessel's correction). Requires at least two values.
    Sample,
};

/// Arithmetic mean. Throws std::invalid_argument when `values` is empty.
[[nodiscard]] double mean(std::span<const double> values);

/// Variance around the mean. Two-pass summation, so the result does not depend
/// on the order of the values in a way that naive one-pass code would.
///
/// Throws std::invalid_argument when `values` is empty, or when sample variance
/// is requested with fewer than two values.
[[nodiscard]] double variance(std::span<const double> values,
                              Ddof ddof = Ddof::Population);

/// Root mean square: sqrt(sum(x^2) / N). Throws std::invalid_argument when empty.
[[nodiscard]] double rms(std::span<const double> values);

struct ColumnStats {
    std::size_t count = 0;
    double mean = 0.0;
    double variance = 0.0;
    double rms = 0.0;
};

/// All four numbers in one pass over `values`. Same preconditions as `variance`.
[[nodiscard]] ColumnStats compute_column_stats(std::span<const double> values,
                                               Ddof ddof = Ddof::Population);

  }  // namespace sensekit::stats
