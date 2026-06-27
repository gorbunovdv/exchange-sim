find_package(GTest REQUIRED)
include(GoogleTest)

add_subdirectory(logging)
add_subdirectory(config)

add_library(base run.cpp)
target_link_libraries(base PUBLIC config logging)

add_executable(empty_app empty_app.cpp)
target_link_libraries(empty_app PUBLIC base)

add_executable(run_test tests/run_test.cpp)
target_link_libraries(run_test PUBLIC base gtest::gtest)
gtest_discover_tests(run_test)

add_executable(temporary_test tests/temporary_test.cpp)
target_link_libraries(temporary_test PUBLIC gtest::gtest)
gtest_discover_tests(temporary_test)
