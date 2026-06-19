#include "../loggable_class.h"
#include "helper.h"
#include <gtest/gtest.h>

TEST(LoggableClass, TestLoggableClass) {
    using namespace exchange_sim;

    struct Structure : LoggableClass {
        Structure(): LoggableClass("Structure") {}

        void log_from_structure(std::string string) {
            INFO("{}", string);
        }
    };

    auto temp_directory = std::filesystem::temp_directory_path();

    std::filesystem::path log_path = temp_directory / "log.txt";

    LoggingBackend::init(LoggingBackend::LoggingConfig{
        .type = LoggingBackend::LoggingConfig::Type::FILE,
        .path = log_path,
        .log_level = quill::LogLevel::Info,
        .format = quill::PatternFormatterOptions("{{%(logger)}} %(message)")
    });

    std::array lines = {"ababc", "fdhsdfhs", "dsjfsdjhfj", "dkfkjsdf"};
    std::string expected_logs = "";

    Structure structure;

    for (auto i : lines) {
        structure.log_from_structure(i);
        expected_logs += std::format("{{Structure}} {}\n", i);
    }

    LoggingBackend::instance().get_logger()->flush_log();

    std::string logs = read_file(log_path);
    std::filesystem::remove(log_path);

    EXPECT_EQ(logs, expected_logs);
}