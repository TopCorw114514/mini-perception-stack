#include <gtest/gtest.h>

#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>

#include "sensekit/io/load_numeric_table.hpp"

namespace {

using sensekit::io::HeaderMode;
using sensekit::io::LoadError;
using sensekit::io::LoadErrorKind;
using sensekit::io::LoadOptions;
using sensekit::io::parse_numeric_table;

struct CapturedError {
    LoadErrorKind kind;
    std::size_t line;
};

template <class Fn>
[[nodiscard]] std::optional<CapturedError> capture_error(Fn&& action) {
    try {
        action();
    } catch (const LoadError& error) {
        return CapturedError{error.kind(), error.line_number()};
    }
    return std::nullopt;
}

TEST(LoadNumericTable, ReadsCommaSeparatedFileWithHeader) {
    const auto table = parse_numeric_table("a,b\n1,2\n3,4\n");
    ASSERT_EQ(table.column_count(), 2u);
    ASSERT_EQ(table.row_count(), 2u);
    EXPECT_EQ(table.column_name(0), "a");
    EXPECT_EQ(table.column_name(1), "b");
    EXPECT_DOUBLE_EQ(table.row(0)[0], 1.0);
    EXPECT_DOUBLE_EQ(table.row(1)[1], 4.0);
}

TEST(LoadNumericTable, NamesColumnsWhenThereIsNoHeader) {
    const auto table = parse_numeric_table("1 2 3\n4 5 6\n");
    ASSERT_EQ(table.column_count(), 3u);
    EXPECT_EQ(table.column_name(0), "column_0");
    EXPECT_EQ(table.column_name(2), "column_2");
    EXPECT_FALSE(table.has_column("column_3"));
}

TEST(LoadNumericTable, AutoDetectsTheCommaDelimiter) {
    const auto table = parse_numeric_table("1,2\n3,4\n");
    ASSERT_EQ(table.column_count(), 2u);
    EXPECT_DOUBLE_EQ(table.row(1)[0], 3.0);
}

TEST(LoadNumericTable, SkipsCommentsAndBlankLines) {
    const auto table = parse_numeric_table("# a comment\n\n1 2\n\n   # indented comment\n3 4\n");
    ASSERT_EQ(table.row_count(), 2u);
    EXPECT_DOUBLE_EQ(table.row(1)[1], 4.0);
}

TEST(LoadNumericTable, AcceptsCarriageReturns) {
    const auto table = parse_numeric_table("1 2\r\n3 4\r\n");
    ASSERT_EQ(table.row_count(), 2u);
    EXPECT_DOUBLE_EQ(table.row(0)[1], 2.0);
}

TEST(LoadNumericTable, AcceptsATabSeparatedFile) {
    const auto table = parse_numeric_table("1\t2\n3\t4\n");
    ASSERT_EQ(table.column_count(), 2u);
    EXPECT_DOUBLE_EQ(table.row(1)[1], 4.0);
}

TEST(LoadNumericTable, IgnoresAByteOrderMark) {
    // The literal is split on purpose: in "\xBF1" the compiler would read `1`
    // as another hex digit, which is exactly the kind of bug the test below
    // would otherwise hide.
    const auto table = parse_numeric_table("\xEF\xBB\xBF" "1 2\n3 4\n");
    ASSERT_EQ(table.row_count(), 2u);
    EXPECT_EQ(table.column_name(0), "column_0");
}

TEST(LoadNumericTable, TrimsSpacesAroundCommaSeparatedFields) {
    const auto table = parse_numeric_table(" 1 , 2 \n3,4\n");
    ASSERT_EQ(table.column_count(), 2u);
    EXPECT_DOUBLE_EQ(table.row(0)[0], 1.0);
    EXPECT_DOUBLE_EQ(table.row(0)[1], 2.0);
}

TEST(LoadNumericTable, ReadsScientificNotation) {
    const auto table = parse_numeric_table("1e-3 -2.5E2\n");
    EXPECT_DOUBLE_EQ(table.row(0)[0], 1e-3);
    EXPECT_DOUBLE_EQ(table.row(0)[1], -250.0);
}

TEST(LoadNumericTable, TrailingNewlineDoesNotCreateAnExtraRow) {
    const auto table = parse_numeric_table("1 2\n");
    EXPECT_EQ(table.row_count(), 1u);
}

TEST(LoadNumericTable, KeepsTheSourceName) {
    const auto table = parse_numeric_table("1 2\n", {}, "signals.txt");
    EXPECT_EQ(table.source_path(), "signals.txt");
}

TEST(LoadNumericTable, HeaderCanBeForcedOnTextFreeFirstRow) {
    LoadOptions options;
    options.header = HeaderMode::FirstRow;
    const auto table = parse_numeric_table("1 2\n3 4\n", options);
    EXPECT_EQ(table.column_name(0), "1");
    EXPECT_EQ(table.row_count(), 1u);
}

TEST(LoadNumericTable, HeaderCanBeDisabled) {
    LoadOptions options;
    options.header = HeaderMode::None;
    const auto error = capture_error([&] { (void)parse_numeric_table("a,b\n1,2\n", options); });
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error->kind, LoadErrorKind::NonNumericToken);
    EXPECT_EQ(error->line, 1u);
}

TEST(LoadNumericTable, ReportsARaggedRowWithItsLineNumber) {
    const auto error = capture_error([] { (void)parse_numeric_table("1 2\n3\n"); });
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error->kind, LoadErrorKind::MalformedRow);
    EXPECT_EQ(error->line, 2u);
}

TEST(LoadNumericTable, ReportsANonNumericField) {
    const auto error = capture_error([] { (void)parse_numeric_table("1 2\n3 x\n"); });
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error->kind, LoadErrorKind::NonNumericToken);
    EXPECT_EQ(error->line, 2u);
}

TEST(LoadNumericTable, ReportsAnEmptyCommaField) {
    // Header detection is disabled on purpose: with the default `Auto` mode the
    // first line of an all-broken file looks like a header, and the error would
    // be "no data rows" instead of the field that is actually wrong.
    LoadOptions options;
    options.header = HeaderMode::None;
    const auto error = capture_error([&] { (void)parse_numeric_table("1,,3\n", options); });
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error->kind, LoadErrorKind::NonNumericToken);
}

TEST(LoadNumericTable, RejectsALeadingPlusSign) {
    // std::from_chars is intentionally strict: this documents the behaviour
    // instead of letting the difference show up as a mystery later.
    LoadOptions options;
    options.header = HeaderMode::None;
    const auto error = capture_error([&] { (void)parse_numeric_table("+1 2\n", options); });
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error->kind, LoadErrorKind::NonNumericToken);
}

TEST(LoadNumericTable, ATypedFirstLineIsTreatedAsAHeader) {
    // With the default `Auto` mode a first line that is not all numbers is read
    // as a header, so a file that is broken from the very first line reports
    // "no data rows" rather than a field level error. Both messages are useful,
    // and knowing which one you will get saves debugging time.
    const auto error = capture_error([] { (void)parse_numeric_table("+1 2\n"); });
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error->kind, LoadErrorKind::EmptyInput);
}

TEST(LoadNumericTable, ReportsAHeaderThatDoesNotMatchTheData) {
    const auto error = capture_error([] { (void)parse_numeric_table("a,b\n1,2,3\n"); });
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error->kind, LoadErrorKind::MalformedRow);
}

TEST(LoadNumericTable, ReportsAnEmptyInput) {
    const auto error = capture_error([] { (void)parse_numeric_table("\n\n# only comments\n"); });
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error->kind, LoadErrorKind::EmptyInput);
}

TEST(LoadNumericTable, KeepsNanByDefaultButRejectsItInStrictMode) {
    const auto table = parse_numeric_table("1 nan\n");
    EXPECT_TRUE(std::isnan(table.row(0)[1]));

    LoadOptions strict;
    strict.reject_non_finite = true;
    const auto error = capture_error([&] { (void)parse_numeric_table("1 nan\n", strict); });
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error->kind, LoadErrorKind::NonFiniteValue);
    EXPECT_EQ(error->line, 1u);
}

TEST(LoadNumericTable, ExposesOneColumnAsAVector) {
    const auto table = parse_numeric_table("1 2\n3 4\n5 6\n");
    const auto column = table.column(1);
    ASSERT_EQ(column.size(), 3u);
    EXPECT_DOUBLE_EQ(column[2], 6.0);
    EXPECT_EQ(table.find_column("column_1"), 1u);
    EXPECT_THROW((void)table.column(9), std::out_of_range);
}

}  // namespace
