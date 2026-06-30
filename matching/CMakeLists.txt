add_library(matching simple_l3book.cpp risk_checker.cpp matching_engine.h)
target_link_libraries(matching PUBLIC base domain)
