#include "base/logging/loggable_class.h"
#include "base/logging/logging_backend.h"
#include "base/temporary.h"
#include "base/read_file.h"

#include <gtest/gtest.h>
#include <string_view>

using namespace exchange_sim;

TEST(LoggableClass, BasicLoggableClassTest) {
  struct Structure : LoggableClass {
    Structure(): LoggableClass("Structure") {}

    void log_from_structure(std::string_view view) { INFO("{}", view); }
  };

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

  Structure structure;

  for (auto i : lines) {
    structure.log_from_structure(i);
    expected_logs += std::format("{{Structure}} {}\n", i);
  }

  structure.flush_log();

  std::string logs = read_file(file.path());

  EXPECT_EQ(logs, expected_logs);
}