find_package(quill REQUIRED)
find_package(magic_enum REQUIRED)

add_library(logging loggable_class.cpp logging_backend.cpp)
target_link_libraries(logging PUBLIC quill::quill magic_enum::magic_enum)

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(loggable_class_test tests/test_loggable_class.cpp)
target_link_libraries(loggable_class_test PUBLIC logging gtest::gtest)
gtest_discover_tests(loggable_class_test)

add_executable(logging_backend_test tests/test_logging_backend.cpp)
target_link_libraries(logging_backend_test PUBLIC logging gtest::gtest)
gtest_discover_tests(logging_backend_test)

