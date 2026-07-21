find_package(glaze REQUIRED)

add_library(exchange_dispatcher INTERFACE exchange_dispatcher.h)
target_link_libraries(exchange_dispatcher INTERFACE exchange_domain glaze::glaze)
target_sources(exchange_dispatcher PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES exchange_dispatcher.h
)
