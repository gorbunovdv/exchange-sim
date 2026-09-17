add_library(allocators INTERFACE)
target_link_libraries(allocators INTERFACE logging Boost::boost)
target_sources(allocators PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES place_arena.h custom_allocator.h
)