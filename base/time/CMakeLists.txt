add_library(time rdtsc.cpp)

add_executable(rdtsc_test tests/rdtsc_test.cpp)
target_link_libraries(rdtsc_test PUBLIC time gtest::gtest)
gtest_discover_tests(rdtsc_test DISCOVERY_MODE PRE_TEST)