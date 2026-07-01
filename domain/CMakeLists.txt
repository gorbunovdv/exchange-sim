find_package(Boost REQUIRED)
find_package(magic_enum REQUIRED)
find_package(GTest REQUIRED)

add_library(domain engine_snapshots.h market_data_snapshots.h reports.h requests.h security.h types.h security_storage.cpp)
target_link_libraries(domain PUBLIC base Boost::boost magic_enum::magic_enum)

add_executable(security_storage_test tests/security_storage_test.cpp)
target_link_libraries(security_storage_test PUBLIC domain gtest::gtest)
gtest_discover_tests(security_storage_test)
