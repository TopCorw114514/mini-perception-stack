cmake_minimum_required(VERSION 3.25)

project(mini_perception_stack
    VERSION 0.1.0
    DESCRIPTION "A miniature perception stack: from raw IMU files to activity predictions"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# High warning level, but never warnings-as-errors: this repository is also a
# learning sandbox and a red build should not stop the lesson.
#
# The options live on an INTERFACE target instead of add_compile_options() so
# that third party code pulled in by FetchContent keeps its own warning level.
add_library(sensekit_warnings INTERFACE)
if(MSVC)
    target_compile_options(sensekit_warnings INTERFACE /W4 /permissive- /Zc:__cplusplus /utf-8)
else()
    target_compile_options(sensekit_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

# ------------------------------------------------------------------ library ---
add_library(sensekit
    src/io/numeric_table.cpp
    src/io/load_numeric_table.cpp
    src/stats/stats.cpp
    src/cli/stats_cli.cpp)
add_library(sensekit::sensekit ALIAS sensekit)

target_include_directories(sensekit
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")
target_compile_features(sensekit PUBLIC cxx_std_20)
target_link_libraries(sensekit PRIVATE sensekit_warnings)

# --------------------------------------------------------------- executable ---
add_executable(sensekit-stats apps/sensekit_stats/main.cpp)
target_link_libraries(sensekit-stats PRIVATE sensekit::sensekit sensekit_warnings)

# -------------------------------------------------------------------- tests ---
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
