find_package(mimalloc REQUIRED)

add_library(run_single_threaded_callback_app INTERFACE)
target_link_libraries(run_single_threaded_callback_app INTERFACE base domain exchange_domain)
target_sources(run_single_threaded_callback_app PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES run_single_threaded_callback_app.h
)

add_executable(simple_cb_app simple_cb_app.cpp)
target_link_libraries(simple_cb_app PUBLIC base run_single_threaded_callback_app simple_order_generator matching exchange_dispatcher)

add_library(run_single_threaded_event_loop_app INTERFACE)
target_link_libraries(run_single_threaded_event_loop_app INTERFACE base domain exchange_domain)
target_sources(run_single_threaded_event_loop_app PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES run_single_threaded_event_loop_app.h
)

add_executable(simple_el_app simple_el_app.cpp)
target_link_libraries(simple_el_app PUBLIC base run_single_threaded_event_loop_app simple_order_generator matching exchange_dispatcher)

add_executable(empty_app empty_app.cpp)
target_link_libraries(empty_app PUBLIC base exchange_domain simple_order_generator matching exchange_dispatcher)

add_library(run_multi_threaded_app INTERFACE)
target_link_libraries(run_multi_threaded_app INTERFACE base domain exchange_domain)
target_sources(run_multi_threaded_app PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES run_multi_threaded_app.h
)

add_executable(simple_mt_app simple_mt_app.cpp)
target_link_libraries(simple_mt_app PUBLIC base run_multi_threaded_app simple_order_generator matching exchange_dispatcher)

add_executable(calc_stats calc_stats.cpp)
target_link_libraries(calc_stats PUBLIC base exchange_domain)
