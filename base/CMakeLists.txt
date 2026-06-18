find_package(GTest REQUIRED)

add_executable(empty_test empty_test.cpp)
target_link_libraries(empty_test PUBLIC gtest::gtest)

include(GoogleTest)
gtest_discover_tests(empty_test)