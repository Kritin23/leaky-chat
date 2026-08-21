# Find Google Test (installed via Homebrew)
include(GoogleTest)
find_package(GTest REQUIRED)

# Create test executable
add_executable(serialisation_test serialisation_test.cc)

# Include header paths from utils (now at ../utils relative to tests)
target_include_directories(serialisation_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../utils)

# Link GTest and your utils library
target_link_libraries(serialisation_test
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    utils
)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(serialisation_test)