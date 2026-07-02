find_package(glaze REQUIRED)

add_library(exchange_dispatcher INTERFACE exchange_dispatcher.h)
target_link_libraries(exchange_dispatcher INTERFACE domain glaze::glaze)

add_library(market_data_generator INTERFACE market_data_generator.h)
target_link_libraries(market_data_generator INTERFACE domain glaze::glaze)

add_library(report_generator INTERFACE report_generator.h)
target_link_libraries(report_generator INTERFACE domain glaze::glaze)