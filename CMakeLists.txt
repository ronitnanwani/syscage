cmake_minimum_required(VERSION 3.16)
project(syscage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Main executable ---
add_executable(syscage
    src/main.cpp
    src/container.cpp
    src/hostname.cpp
    src/mounts.cpp
    src/capabilities.cpp
    src/syscalls.cpp
    src/resources.cpp
    src/user_namespace.cpp
)

target_include_directories(syscage PRIVATE include)
target_link_libraries(syscage PRIVATE cap seccomp)

# --- Tests ---
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
# Prevent overriding parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Library of all source modules (excluding main.cpp) for tests to link against
add_library(syscage_lib STATIC
    src/container.cpp
    src/hostname.cpp
    src/mounts.cpp
    src/capabilities.cpp
    src/syscalls.cpp
    src/resources.cpp
    src/user_namespace.cpp
)
target_include_directories(syscage_lib PUBLIC include)
target_link_libraries(syscage_lib PUBLIC cap seccomp)

# Test executable
add_executable(syscage_tests
    tests/test_hostname.cpp
    tests/test_config.cpp
    tests/test_resources.cpp
    tests/test_capabilities.cpp
    tests/test_syscalls.cpp
    tests/test_mounts.cpp
    tests/test_container.cpp
)

target_link_libraries(syscage_tests PRIVATE syscage_lib GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(syscage_tests)
