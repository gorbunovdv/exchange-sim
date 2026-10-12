find_package(Boost REQUIRED)
find_package(cpptrace REQUIRED)
find_package(mimalloc REQUIRED)

add_library(allocators allocator_tracking.cpp)
target_link_libraries(allocators PUBLIC mimalloc-static logging Boost::boost cpptrace::cpptrace)
target_sources(allocators PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES place_arena.h custom_allocator.h allocator_tracking.h
)

add_executable(allocator_tracking_test tests/allocator_tracking_test.cpp)
target_link_libraries(allocator_tracking_test PUBLIC allocators gtest::gtest)
gtest_discover_tests(allocator_tracking_test DISCOVERY_MODE PRE_TEST)
