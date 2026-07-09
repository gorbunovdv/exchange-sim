find_package(GTest REQUIRED)
find_package(Boost REQUIRED)
find_package(cpptrace REQUIRED)
include(GoogleTest)

add_subdirectory(threading)
add_subdirectory(logging)
add_subdirectory(config)
add_subdirectory(time)

add_library(base run.cpp threading/core_allocator.cpp callback_sink.h counter.h macros.h noncopyable.h read_file.h callback_receiver.h temporary.h)
target_link_libraries(base PUBLIC config logging time Boost::boost cpptrace::cpptrace)

add_executable(run_test tests/run_test.cpp)
target_link_libraries(run_test PUBLIC base gtest::gtest)
gtest_discover_tests(run_test)

add_executable(temporary_test tests/temporary_test.cpp)
target_link_libraries(temporary_test PUBLIC gtest::gtest)
gtest_discover_tests(temporary_test)
