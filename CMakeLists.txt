cmake_minimum_required(VERSION 3.20)
project(poker_eval LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
)
target_include_directories(poker_core PUBLIC include)
target_link_libraries(poker_core PRIVATE project_warnings)

add_executable(gen src/main.cpp)
target_link_libraries(gen PRIVATE poker_core project_warnings)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
