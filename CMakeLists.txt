cmake_minimum_required(VERSION 3.10)
project(KVStorage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libs/gtest)

add_library(kv_storage INTERFACE)
target_include_directories(kv_storage INTERFACE inc)

enable_testing()

add_executable(kv_storage_tests
    tests/kv_storage_test.cpp
)

target_link_libraries(kv_storage_tests
    kv_storage
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(kv_storage_tests)
