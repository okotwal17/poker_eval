include(FetchContent)

# Downloads and builds Catch2 as part of this project the first time you
# configure. No system-wide install, and the version is pinned right here.
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(tests
    test_evaluate.cpp
    test_combos.cpp
)
target_link_libraries(tests PRIVATE poker_core project_warnings Catch2::Catch2WithMain)

# Absolute path to the ranking CSV, so the exhaustive test can find it no matter
# which directory the test binary is launched from.
target_compile_definitions(tests PRIVATE
    RANKINGS_CSV="${CMAKE_SOURCE_DIR}/poker_7462_rankings.csv"
)

# Catch ships a helper that registers each TEST_CASE with ctest individually.
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(tests)
