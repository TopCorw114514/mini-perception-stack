#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "sensekit/cli/stats_cli.hpp"

namespace {

namespace fs = std::filesystem;

constexpr int kSuccess = 0;
constexpr int kUsageOrIoError = 2;
constexpr int kMalformedData = 3;

[[nodiscard]] fs::path data_file(const std::string& name) {
    // CMake bakes this path in as a UTF-8 string (the project compiles with
    // /utf-8). Building a std::filesystem::path straight from those bytes would
    // decode them with the active Windows code page instead, which silently
    // breaks as soon as the path contains a non-ASCII character - which is
    // exactly what this repository did after it was moved into a folder with
    // Chinese characters in its name. Going through char8_t states the encoding
    // instead of hoping for the right default.
    static const fs::path root = [] {
        const auto* utf8 = reinterpret_cast<const char8_t*>(SENSEKIT_TEST_DATA_DIR);
        return fs::path(std::u8string(utf8));
    }();
    return root / name;
}

[[nodiscard]] std::string read_text(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

struct CliResult {
    int code = 0;
    std::string out;
    std::string err;
};

[[nodiscard]] CliResult run_cli(const std::vector<std::string>& args) {
    std::ostringstream out;
    std::ostringstream err;
    CliResult result;
    result.code = sensekit::cli::run_stats_cli(args, out, err);
    result.out = out.str();
    result.err = err.str();
    return result;
}

[[nodiscard]] fs::path write_temp_file(const std::string& name, const std::string& content) {
    const auto directory = fs::temp_directory_path() / "sensekit-stats-tests";
    fs::create_directories(directory);
    const auto path = directory / name;
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << content;
    return path;
}

[[nodiscard]] std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

TEST(StatsCli, HelpGoesToStdoutAndSucceeds) {
    const auto result = run_cli({"--help"});
    EXPECT_EQ(result.code, kSuccess);
    EXPECT_NE(result.out.find("usage:"), std::string::npos);
}

TEST(StatsCli, WithoutAnInputFileTheUsageIsPrinted) {
    const auto result = run_cli({});
    EXPECT_EQ(result.code, kUsageOrIoError);
    EXPECT_NE(result.err.find("no input file"), std::string::npos);
}

TEST(StatsCli, AMissingFileIsAUsageError) {
    const auto result = run_cli({data_file("does_not_exist.txt").string()});
    EXPECT_EQ(result.code, kUsageOrIoError);
    EXPECT_NE(result.err.find("cannot open input file"), std::string::npos);
}

TEST(StatsCli, AnUnknownOptionIsAUsageError) {
    const auto result = run_cli({"signals.txt", "--nope"});
    EXPECT_EQ(result.code, kUsageOrIoError);
    EXPECT_NE(result.err.find("unknown option"), std::string::npos);
}

TEST(StatsCli, SpaceSeparatedFixtureMatchesTheGoldenCsv) {
    const auto result = run_cli({data_file("space_no_header.txt").string()});
    ASSERT_EQ(result.code, kSuccess) << result.err;
    EXPECT_EQ(result.out, read_text(data_file("space_no_header.expected.csv")));
}

TEST(StatsCli, CommaSeparatedFixtureMatchesTheGoldenCsv) {
    const auto result = run_cli({data_file("comma_header.csv").string()});
    ASSERT_EQ(result.code, kSuccess) << result.err;
    EXPECT_EQ(result.out, read_text(data_file("comma_header.population.expected.csv")));
}

TEST(StatsCli, SampleVarianceMatchesTheGoldenCsv) {
    const auto result = run_cli({data_file("comma_header.csv").string(), "--ddof", "1"});
    ASSERT_EQ(result.code, kSuccess) << result.err;
    EXPECT_EQ(result.out, read_text(data_file("comma_header.sample.expected.csv")));
}

TEST(StatsCli, CommentedFixtureMatchesTheGoldenCsv) {
    const auto result = run_cli({data_file("commented.txt").string()});
    ASSERT_EQ(result.code, kSuccess) << result.err;
    EXPECT_EQ(result.out, read_text(data_file("commented.expected.csv")));
}

TEST(StatsCli, ColumnNamesSelectASubset) {
    const auto result = run_cli({data_file("comma_header.csv").string(), "--columns", "rms"});
    ASSERT_EQ(result.code, kSuccess) << result.err;
    const auto lines = split_lines(result.out);
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1].substr(0, 4), "rms,");
}

TEST(StatsCli, ColumnIndicesSelectASubset) {
    const auto result = run_cli({data_file("space_no_header.txt").string(), "--columns", "1"});
    ASSERT_EQ(result.code, kSuccess) << result.err;
    const auto lines = split_lines(result.out);
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1].substr(0, 9), "column_1,");
}

TEST(StatsCli, AnUnknownColumnNameIsAUsageError) {
    const auto result = run_cli({data_file("comma_header.csv").string(), "--columns", "nope"});
    EXPECT_EQ(result.code, kUsageOrIoError);
    EXPECT_NE(result.err.find("is not a column"), std::string::npos);
}

TEST(StatsCli, AnOutOfRangeColumnIndexIsAUsageError) {
    const auto result = run_cli({data_file("comma_header.csv").string(), "--columns", "9"});
    EXPECT_EQ(result.code, kUsageOrIoError);
    EXPECT_NE(result.err.find("out of range"), std::string::npos);
}

TEST(StatsCli, ARaggedFileIsMalformedData) {
    const auto result = run_cli({data_file("ragged.txt").string()});
    EXPECT_EQ(result.code, kMalformedData);
    // Diagnostics are printed as `<file>:<line>: <what went wrong>`.
    EXPECT_NE(result.err.find(":2:"), std::string::npos);
    EXPECT_NE(result.err.find("expected 3 columns"), std::string::npos);
}

TEST(StatsCli, ANonNumericFileIsMalformedData) {
    const auto result = run_cli({data_file("non_numeric.txt").string()});
    EXPECT_EQ(result.code, kMalformedData);
    EXPECT_NE(result.err.find("is not a number"), std::string::npos);
}

TEST(StatsCli, StrictModeRejectsNonFiniteValues) {
    const auto path = write_temp_file("non_finite.txt", "1 nan\n2 3\n");
    EXPECT_EQ(run_cli({path.string()}).code, kSuccess);

    const auto strict = run_cli({path.string(), "--strict"});
    EXPECT_EQ(strict.code, kMalformedData);
    EXPECT_NE(strict.err.find("finite"), std::string::npos);
}

TEST(StatsCli, OutputFileMatchesStdout) {
    const auto stdout_result = run_cli({data_file("comma_header.csv").string()});
    ASSERT_EQ(stdout_result.code, kSuccess) << stdout_result.err;

    const auto output_path =
        fs::temp_directory_path() / "sensekit-stats-tests" / "stdout_vs_file.csv";
    const auto file_result =
        run_cli({data_file("comma_header.csv").string(), "--output", output_path.string()});
    ASSERT_EQ(file_result.code, kSuccess) << file_result.err;
    EXPECT_TRUE(file_result.out.empty());
    EXPECT_EQ(read_text(output_path), stdout_result.out);
}

TEST(StatsCli, HandlesAWideSpaceSeparatedTableLikeUciHar) {
    // The real UCI HAR files are space separated, headerless and 128 columns
    // wide, with 7352 rows. The shape is what matters here, so the fixture is
    // generated instead of committing an eight megabyte text file.
    std::string content;
    for (int row = 0; row < 6; ++row) {
        for (int column = 0; column < 128; ++column) {
            if (column > 0) {
                content += ' ';
            }
            content += std::to_string(column * 3 + row);
        }
        content += '\n';
    }

    const auto path = write_temp_file("wide_128_columns.txt", content);
    const auto result = run_cli({path.string()});
    ASSERT_EQ(result.code, kSuccess) << result.err;

    const auto lines = split_lines(result.out);
    ASSERT_EQ(lines.size(), 129u);
    EXPECT_EQ(lines.front(), "column,count,mean,variance,rms");
    EXPECT_EQ(lines[1].substr(0, 9), "column_0,");
    EXPECT_EQ(lines.back().substr(0, 11), "column_127,");
    EXPECT_EQ(result.out.find("nan"), std::string::npos);
    EXPECT_EQ(result.out.find("inf"), std::string::npos);
}

}  // namespace
