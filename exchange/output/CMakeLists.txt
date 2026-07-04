find_package(glaze REQUIRED)

add_library(exchange_dispatcher INTERFACE exchange_dispatcher.h)
target_link_libraries(exchange_dispatcher INTERFACE domain glaze::glaze)
