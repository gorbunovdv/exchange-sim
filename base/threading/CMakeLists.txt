find_package(Boost REQUIRED)

add_library(threading core_allocator.cpp)
target_link_libraries(threading PUBLIC logging Boost::boost)
