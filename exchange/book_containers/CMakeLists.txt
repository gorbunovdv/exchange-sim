add_library(book_containers INTERFACE)
target_link_libraries(book_containers INTERFACE base exchange_domain)
target_sources(book_containers INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES stl_book_container.h book_container_concept.h)