find_package(GTest REQUIRED)
find_package(glaze REQUIRED)

add_library(config INTERFACE config.h)
target_link_libraries(config INTERFACE glaze::glaze)

add_executable(config_test tests/config_test.cpp)
target_link_libraries(config_test PUBLIC config gtest::gtest)
include(GoogleTest)
gtest_discover_tests(config_test)