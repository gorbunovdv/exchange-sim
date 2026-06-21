find_package(GTest REQUIRED)
include(GoogleTest)

add_subdirectory(logging)
add_subdirectory(config)

add_executable(test_run tests/test_run.cpp)
target_link_libraries(test_run PUBLIC base gtest::gtest)
gtest_discover_tests(test_run)

add_library(base run.cpp)
target_link_libraries(base PUBLIC config logging)

add_executable(empty_app empty_app.cpp)
target_link_libraries(empty_app PUBLIC base)
