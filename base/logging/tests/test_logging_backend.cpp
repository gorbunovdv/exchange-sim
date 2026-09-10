#include "base/logging/logging_backend.h"
#include "base/read_file.h"
#include "base/temporary.h"

#include <gtest/gtest.h>

#include <numeric>
#include <optional>

using namespace exchange_sim;

struct LoggingBackendCorePinningTestInput {
  std::optional<CpuID> cpu_id;
  std::string name;
};

std::ostream& operator<<(std::ostream& stream, const LoggingBackendCorePinningTestInput& input) {
  return stream << input.name;
}

class LoggingBackendCorePinningTest : public testing::TestWithParam<LoggingBackendCorePinningTestInput> {};

TEST_P(LoggingBackendCorePinningTest, BasicLoggingBackendTest) {
  TemporaryDirectory directory("logs");
  TemporaryFile file(directory, "logs");

  LoggingBackend::init(LoggingBackend::LoggingConfig{
      .type = LoggingBackend::LoggingConfig::Type::FILE,
      .path = file.path(),
      .log_level = quill::LogLevel::Info,
      .format = quill::PatternFormatterOptions("{{%(logger)}} %(message)"),
      .cpu_id = GetParam().cpu_id,
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

INSTANTIATE_TEST_SUITE_P(LoggingBackendTests,
    LoggingBackendCorePinningTest,
    testing::Values(LoggingBackendCorePinningTestInput{std::optional<CpuID>{CpuID{0}}, "Cpu0"},
        LoggingBackendCorePinningTestInput{std::nullopt, "NoCpu"}));

TEST(LoggingBackendTests, CriticalTest) {
  auto log_critical = []() {
    CRITICAL("Critical message");
  };

  EXPECT_THROW(log_critical(), std::runtime_error);
}
