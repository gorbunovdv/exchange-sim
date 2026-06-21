#include "helper.h"
#include "base/logging/logging_backend.h"
#include "base/temporary.h"
#include <gtest/gtest.h>
#include <numeric>

TEST(LoggingBackendTests, BasicLoggingBackendTest) {
  using namespace exchange_sim;

  TemporaryFile file("logs");

  LoggingBackend::init(LoggingBackend::LoggingConfig{
      .type = LoggingBackend::LoggingConfig::Type::FILE,
      .path = file.path(),
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

  std::string logs = read_file(file.path());

  EXPECT_EQ(logs, expected_logs);
}