find_package(GTest REQUIRED)

add_library(matching simple_l3book.cpp risk_checker.cpp matching_engine.h)
target_link_libraries(matching PUBLIC base domain)

add_executable(risk_checker_test tests/risk_checker_test.cpp)
target_link_libraries(risk_checker_test PUBLIC matching gtest::gtest)
gtest_discover_tests(risk_checker_test)

add_executable(matching_engine_test tests/matching_engine_test.cpp)
target_link_libraries(matching_engine_test PUBLIC matching gtest::gtest)
gtest_discover_tests(matching_engine_test)