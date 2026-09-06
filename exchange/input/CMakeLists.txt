add_library(abstract_order_generator order_storage.cpp debug_orders_skill.cpp store_orders_skill.cpp)
target_link_libraries(abstract_order_generator PUBLIC base exchange_domain book_containers)
target_sources(abstract_order_generator PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES order_generator_wrapper.h order_storage.h simple_order_generator.h generator_interface.h
        store_orders_skill.h debug_orders_skill.h maintain_book_skill.h shift_order_generator.h pending_transactions_skill.h
)

add_executable(order_storage_test tests/order_storage_test.cpp)
target_link_libraries(order_storage_test PUBLIC abstract_order_generator gtest::gtest)
gtest_discover_tests(order_storage_test DISCOVERY_MODE PRE_TEST)

add_library(simple_order_generator INTERFACE)
target_link_libraries(simple_order_generator INTERFACE abstract_order_generator)
target_sources(simple_order_generator INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES simple_order_generator.h
)

add_library(shift_order_generator INTERFACE)
target_link_libraries(shift_order_generator INTERFACE abstract_order_generator)
target_sources(shift_order_generator INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES shift_order_generator.h
)