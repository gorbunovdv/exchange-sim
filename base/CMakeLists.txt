find_package(GTest REQUIRED)
find_package(spdlog REQUIRED)

add_executable(test_log_write test_log_write.cpp)
target_link_libraries(test_log_write PUBLIC spdlog::spdlog gtest::gtest)

include(GoogleTest)
gtest_discover_tests(test_log_write)