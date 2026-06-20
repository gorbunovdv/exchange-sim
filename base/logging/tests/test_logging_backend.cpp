#include "helper.h"
#include "base/logging/logging_backend.h"
#include <gtest/gtest.h>
#include <numeric>

TEST(LoggingBackendTests, LoggingBackend) {
  using namespace exchange_sim;

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
  for (auto i : lines) {
    INFO("{}", i);
    expected_logs += std::format("{{root}} {}\n", i);
  }

  LoggingBackend::instance().get_logger()->flush_log();

  std::string logs = read_file(log_path);
  std::filesystem::remove(log_path);

  EXPECT_EQ(logs, expected_logs);
}