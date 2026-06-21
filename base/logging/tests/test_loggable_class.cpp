#include "base/temporary.h"
#include "base/logging/loggable_class.h"
#include "helper.h"

#include <gtest/gtest.h>

TEST(LoggableClass, BasicLoggableClassTest) {
  using namespace exchange_sim;

  struct Structure : LoggableClass {
    Structure(): LoggableClass("Structure") {
    }

    void log_from_structure(std::string string) {
      INFO("{}", string);
    }
  };

  TemporaryFile file("logs");

  LoggingBackend::init(LoggingBackend::LoggingConfig{
      .type = LoggingBackend::LoggingConfig::Type::FILE,
      .path = file.path(),
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

  structure.get_logger()->flush_log();

  std::string logs = read_file(file.path());

  EXPECT_EQ(logs, expected_logs);
}