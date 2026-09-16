#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <vector>

#include "sensekit/stats/stats.hpp"

namespace {

using sensekit::stats::compute_column_stats;
using sensekit::stats::Ddof;
using sensekit::stats::mean;
using sensekit::stats::rms;
using sensekit::stats::variance;

constexpr double kTolerance = 1e-12;

TEST(StatsMean, MatchesHandComputedValue) {
    const std::vector<double> values{1.0, 2.0, 3.0, 4.0};
    EXPECT_NEAR(mean(values), 2.5, kTolerance);
}

TEST(StatsMean, HandlesNegativeValues) {
    const std::vector<double> values{-1.5, 2.5};
    EXPECT_NEAR(mean(values), 0.5, kTolerance);
}

TEST(StatsMean, RejectsEmptyInput) {
    const std::vector<double> values;
    EXPECT_THROW((void)mean(values), std::invalid_argument);
}

TEST(StatsVariance, PopulationVarianceMatchesHandComputedValue) {
    // deviations from the mean 2.5 are -1.5, -0.5, 0.5, 1.5 -> sum of squares 5 -> 5/4
    const std::vector<double> values{1.0, 2.0, 3.0, 4.0};
    EXPECT_NEAR(variance(values), 1.25, kTolerance);
}

TEST(StatsVariance, SampleVarianceMatchesHandComputedValue) {
    // same sum of squares, divided by 3 instead of 4
    const std::vector<double> values{1.0, 2.0, 3.0, 4.0};
    EXPECT_NEAR(variance(values, Ddof::Sample), 5.0 / 3.0, kTolerance);
}

TEST(StatsVariance, PopulationIsTheDefault) {
    const std::vector<double> values{1.0, 2.0, 3.0, 4.0};
    EXPECT_DOUBLE_EQ(variance(values), variance(values, Ddof::Population));
}

TEST(StatsVariance, ConstantSignalHasZeroVariance) {
    const std::vector<double> values{5.0, 5.0, 5.0, 5.0};
    EXPECT_DOUBLE_EQ(variance(values), 0.0);
    EXPECT_DOUBLE_EQ(variance(values, Ddof::Sample), 0.0);
}

TEST(StatsVariance, SampleVarianceNeedsTwoValues) {
    const std::vector<double> values{7.0};
    EXPECT_THROW((void)variance(values, Ddof::Sample), std::invalid_argument);
    EXPECT_DOUBLE_EQ(variance(values, Ddof::Population), 0.0);
}

TEST(StatsVariance, RejectsEmptyInput) {
    const std::vector<double> values;
    EXPECT_THROW((void)variance(values), std::invalid_argument);
}

TEST(StatsRms, MatchesHandComputedValue) {
    const std::vector<double> values{3.0, 4.0};
    EXPECT_NEAR(rms(values), std::sqrt(12.5), kTolerance);
}

TEST(StatsRms, IgnoresTheSignOfTheSignal) {
    const std::vector<double> positive{3.0, 4.0};
    const std::vector<double> negative{-3.0, -4.0};
    EXPECT_DOUBLE_EQ(rms(positive), rms(negative));
}

TEST(StatsRms, EqualsStandardDeviationForZeroMeanSignals) {
    const std::vector<double> values{1.0, -1.0, 1.0, -1.0};
    EXPECT_DOUBLE_EQ(mean(values), 0.0);
    EXPECT_NEAR(rms(values), std::sqrt(variance(values)), kTolerance);
}

TEST(StatsRms, RejectsEmptyInput) {
    const std::vector<double> values;
    EXPECT_THROW((void)rms(values), std::invalid_argument);
}

TEST(ColumnStats, FillsEveryField) {
    const std::vector<double> values{1.0, 2.0, 3.0, 4.0};
    const auto summary = compute_column_stats(values);
    EXPECT_EQ(summary.count, 4u);
    EXPECT_NEAR(summary.mean, 2.5, kTolerance);
    EXPECT_NEAR(summary.variance, 1.25, kTolerance);
    EXPECT_NEAR(summary.rms, std::sqrt(7.5), kTolerance);
}

TEST(ColumnStats, HonoursTheRequestedDdof) {
    const std::vector<double> values{1.0, 2.0, 3.0, 4.0};
    EXPECT_NEAR(compute_column_stats(values, Ddof::Population).variance, 1.25, kTolerance);
    EXPECT_NEAR(compute_column_stats(values, Ddof::Sample).variance, 5.0 / 3.0, kTolerance);
}

}  // namespace
