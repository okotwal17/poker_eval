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
)
target_link_libraries(tests PRIVATE poker_core project_warnings Catch2::Catch2WithMain)

# Catch ships a helper that registers each TEST_CASE with ctest individually.
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(tests)
