add_library(exchange_domain INTERFACE)
target_link_libraries(exchange_domain INTERFACE domain)
target_sources(exchange_domain PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES engine_events.h market_events.h rejection_reason.h reports.h requests.h types.h
)
