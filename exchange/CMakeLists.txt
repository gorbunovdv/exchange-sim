find_package(GTest REQUIRED)

add_subdirectory(input)
add_subdirectory(matching)
add_subdirectory(output)
add_subdirectory(application)

add_executable(matching_output_test tests/matching_output_test.cpp)
target_link_libraries(matching_output_test PUBLIC matching exchange_dispatcher gtest::gtest)
gtest_discover_tests(matching_output_test DISCOVERY_MODE PRE_TEST)
