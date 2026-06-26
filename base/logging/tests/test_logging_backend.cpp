#include <gtest/gtest.h>
#include <numeric>
#include "base/logging/logging_backend.h"
#include "base/temporary.h"
#include "base/read_file.h"

TEST(LoggingBackendTests, BasicLoggingBackendTest) {
  using namespace exchange_sim;

  TemporaryDirectory directory("logs");
  TemporaryFile file(directory, "logs");

  LoggingBackend::init(LoggingBackend::LoggingConfig{
      .type = LoggingBackend::LoggingConfig::Type::FILE,
      .path = file.path(),
      .log_level = quill::LogLevel::Info,
      .format = quill::PatternFormatterOptions("{{%(logger)}} %(message)"),
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
  using namespace exchange_sim;

  auto log_critical = [](){ CRITICAL("Critical message"); };

  EXPECT_THROW(log_critical(), std::runtime_error);
}