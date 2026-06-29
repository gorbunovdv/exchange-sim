find_package(Boost REQUIRED)
find_package(cpptrace REQUIRED)

add_executable(single_threaded_app single_threaded_app.cpp)
target_link_libraries(single_threaded_app PUBLIC base Boost::boost cpptrace::cpptrace)
