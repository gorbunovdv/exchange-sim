#include "base/logging/logging_backend.h"
#include "base/read_file.h"
#include "base/temporary.h"

#include <gtest/gtest.h>

#include <numeric>

using namespace exchange_sim;

TEST(LoggingBackendTests, BasicLoggingBackendTest) {
  TemporaryDirectory directory("logs");
  TemporaryFile file(directory, "logs");

  LoggingBackend::init(LoggingBackend::LoggingConfig{
      .type = LoggingBackend::LoggingConfig::Type::FILE,
      .path = file.path(),
      .log_level = quill::LogLevel::Info,
      .format = quill::PatternFormatterOptions("{{%(logger)}} %(message)"),
      .cpu_id = 0,
  });

  std::array lines = {"ababc", "fdhsdfhs", "dsjfsdjhfj", "dkfkjsdf"};
  std::string expected_logs;
  for (auto i : lines) {
    INFO("{}", i);
    expected_logs += std::format("{{root}} {}\n", i);
  }

  LoggingBackend::instance().get_logger()->flush_log();

  std::string logs = read_file(file.path());

  EXPECT_EQ(logs, expected_logs);
}

TEST(LoggingBackendTests, CriticalTest) {
  auto log_critical = []() {
    CRITICAL("Critical message");
  };

  EXPECT_THROW(log_critical(), std::runtime_error);
}