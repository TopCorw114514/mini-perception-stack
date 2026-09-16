include(FetchContent)

# GoogleTest is fetched at configure time and pinned, so every machine and the CI
# runner build the exact same version.
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(sensekit_tests
    test_load_numeric_table.cpp
    test_stats.cpp
    test_stats_cli.cpp)

target_link_libraries(sensekit_tests
    PRIVATE
        sensekit::sensekit
        sensekit_warnings
        GTest::gtest_main)

# Test fixtures stay next to the sources, so the tests also work when the build
# directory is deleted and recreated.
target_compile_definitions(sensekit_tests
    PRIVATE SENSEKIT_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

include(GoogleTest)
gtest_discover_tests(sensekit_tests)
