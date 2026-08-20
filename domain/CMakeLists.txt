find_package(Boost REQUIRED)
find_package(magic_enum REQUIRED)
find_package(benchmark REQUIRED)
find_package(GTest REQUIRED)

add_library(domain security_storage.cpp generate_securities.cpp)
target_link_libraries(domain PUBLIC base Boost::boost magic_enum::magic_enum)
target_sources(domain PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES security.h security_storage.h types.h generate_securities.h
)


add_executable(security_storage_test tests/security_storage_test.cpp)
target_link_libraries(security_storage_test PUBLIC domain gtest::gtest)
gtest_discover_tests(security_storage_test DISCOVERY_MODE PRE_TEST)

add_executable(decimal_benchmark benchmarks/decimal_benchmark.cpp)
target_link_libraries(decimal_benchmark PUBLIC domain benchmark::benchmark_main)
