cmake_minimum_required(VERSION 3.20)
project(poker_eval LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CMake defaults CMAKE_BUILD_TYPE to empty, which means no -O flag at all, so an
# accidental unoptimized build is the default. Default to Release instead, but
# only when the user has not asked for something else, so -DCMAKE_BUILD_TYPE=Debug
# still works.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Writes build/compile_commands.json listing the exact flags used for every
# source file. The VS Code C/C++ extension reads it so IntelliSense sees the
# same include paths and -D defines the compiler does.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Shared warning flags. INTERFACE means this target has no sources of its own,
# it just hands its options to whatever links it, so Catch2 does not inherit them.
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
    -Wall -Wextra -Wimplicit-fallthrough
)

# The actual poker logic. Both the generator and the tests link this, so the
# code is compiled once and neither one owns it.
add_library(poker_core
    src/evaluate.cpp
    src/table.cpp
    src/playPoker.cpp
)
target_include_directories(poker_core PUBLIC include)
target_link_libraries(poker_core PRIVATE project_warnings)

add_executable(gen src/main.cpp)
target_link_libraries(gen PRIVATE poker_core project_warnings)

# Offline search for a perfect hash over the groups keys. Never linked into
# poker_core, it just reads the tables and prints candidates.
add_executable(find_hash tools/find_hash.cpp)
target_link_libraries(find_hash PRIVATE poker_core project_warnings)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
