find_package(GTest REQUIRED)

add_library(matching risk_checker.cpp matching_engine.h)
target_link_libraries(matching PUBLIC base exchange_domain)
target_sources(matching PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES l3book.h matching_engine.h risk_checker.h stl_book_container.h
)

add_executable(risk_checker_test tests/risk_checker_test.cpp)
target_link_libraries(risk_checker_test PUBLIC matching gtest::gtest)
gtest_discover_tests(risk_checker_test DISCOVERY_MODE PRE_TEST)

add_executable(matching_engine_test tests/matching_engine_test.cpp)
target_link_libraries(matching_engine_test PUBLIC matching gtest::gtest)
gtest_discover_tests(matching_engine_test DISCOVERY_MODE PRE_TEST)