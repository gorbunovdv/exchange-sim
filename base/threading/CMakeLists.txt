find_package(Boost REQUIRED)
find_package(cpptrace REQUIRED)

add_library(threading core_allocator.cpp runtime.cpp signals.cpp)
target_link_libraries(threading PUBLIC logging Boost::boost cpptrace::cpptrace)
target_sources(threading PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES cache_line.h core_allocator.h cpu_id.h runtime.h signals.h
)

add_executable(core_allocator_test tests/core_allocator_test.cpp)
target_link_libraries(core_allocator_test PUBLIC threading gtest::gtest)
gtest_discover_tests(core_allocator_test DISCOVERY_MODE PRE_TEST)
