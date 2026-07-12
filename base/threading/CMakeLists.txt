find_package(Boost REQUIRED)
find_package(cpptrace REQUIRED)

add_library(threading core_allocator.cpp runtime.cpp signals.cpp)
target_link_libraries(threading PUBLIC logging Boost::boost cpptrace::cpptrace)
