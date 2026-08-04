find_package(GTest REQUIRED)
find_package(glaze REQUIRED)

add_library(config config.cpp)
target_link_libraries(config PUBLIC glaze::glaze)
target_sources(config PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    FILES config.h
)

add_executable(config_test tests/config_test.cpp)
target_compile_definitions(config_test PRIVATE
    EXCHANGE_SIM_DEFAULT_CONFIG_PATH="${PROJECT_SOURCE_DIR}/base/config/tests/default.json"
)
target_link_libraries(config_test PUBLIC config gtest::gtest)
include(GoogleTest)
gtest_discover_tests(config_test DISCOVERY_MODE PRE_TEST)