find_package(quill REQUIRED)
find_package(magic_enum REQUIRED)

add_library(logging loggable_class.cpp logging_backend.cpp)
target_link_libraries(logging PUBLIC quill::quill magic_enum::magic_enum)
target_sources(logging PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES loggable_class.h logging_backend.h quill_sinks.h
)

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(loggable_class_test tests/test_loggable_class.cpp)
target_link_libraries(loggable_class_test PUBLIC logging gtest::gtest)
gtest_discover_tests(loggable_class_test DISCOVERY_MODE PRE_TEST)

add_executable(logging_backend_test tests/test_logging_backend.cpp)
target_link_libraries(logging_backend_test PUBLIC logging gtest::gtest)
gtest_discover_tests(logging_backend_test DISCOVERY_MODE PRE_TEST)

