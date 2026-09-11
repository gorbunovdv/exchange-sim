find_package(GTest REQUIRED)
find_package(Boost REQUIRED)
find_package(cpptrace REQUIRED)
find_package(mimalloc REQUIRED)
include(GoogleTest)

add_subdirectory(threading)
add_subdirectory(logging)
add_subdirectory(config)
add_subdirectory(time)

add_library(base run.cpp)
target_link_libraries(base PRIVATE mimalloc-static PUBLIC config logging time threading Boost::boost cpptrace::cpptrace)
target_sources(base PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES callback_receiver.h callback_sink.h counter.h forward_sink.h journal.h macros.h noncopyable.h processor.h traits.h
        read_file.h run.h sink.h strong_type.h temporary.h throughput.h indexed_map.h one_of.h thread_receiver.h threaded_journal.h spsc_queue.h random.h
        file.h
)


add_executable(run_test tests/run_test.cpp)
target_link_libraries(run_test PUBLIC base gtest::gtest)
gtest_discover_tests(run_test DISCOVERY_MODE PRE_TEST)

add_executable(temporary_test tests/temporary_test.cpp)
target_link_libraries(temporary_test PUBLIC base gtest::gtest)
gtest_discover_tests(temporary_test DISCOVERY_MODE PRE_TEST)

add_executable(journal_test tests/journal_test.cpp)
target_link_libraries(journal_test PUBLIC base gtest::gtest)
gtest_discover_tests(journal_test DISCOVERY_MODE PRE_TEST)

add_executable(indexed_map_test tests/indexed_map_test.cpp)
target_link_libraries(indexed_map_test PUBLIC base gtest::gtest)
gtest_discover_tests(indexed_map_test DISCOVERY_MODE PRE_TEST)

add_executable(spsc_queue_test tests/spsc_queue_test.cpp)
target_link_libraries(spsc_queue_test PUBLIC base gtest::gtest)
gtest_discover_tests(spsc_queue_test DISCOVERY_MODE PRE_TEST)
