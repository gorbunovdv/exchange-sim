find_package(Boost REQUIRED)
find_package(magic_enum REQUIRED)

add_library(domain exchange_snapshots.h market_data_snapshots.h reports.h requests.h security.h types.h security_storage.cpp)
target_link_libraries(domain PUBLIC base Boost::boost magic_enum::magic_enum)
