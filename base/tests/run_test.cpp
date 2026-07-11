#include "base/logging/logging_backend.h"
#include "base/read_file.h"
#include "base/run.h"
#include "base/temporary.h"

#include <gtest/gtest.h>

#include <fstream>
#include <span>

using namespace exchange_sim;

TEST(Run, MissingConfigFile) {
  TemporaryDirectory directory("run");
  std::string path = TemporaryFile(directory, "config").path().string();

  char* argv[] = {nullptr, path.data()};

  EXPECT_THROW(read_config_and_initialize_logging(std::span{argv, argv + 2}), std::runtime_error);
}

TEST(Run, MissingLoggingType) {
  TemporaryDirectory directory("run");
  TemporaryFile logs_file(directory, "logs");
  TemporaryFile config_file(directory, "config");

  auto config_file_path_string = config_file.path().string();
  auto log_file_path_string = logs_file.path().string();

  static constexpr std::string_view constJsonConfigFormat = R"custom_delimiter(
    {{
      "cpu_cores": {{
        "shared_core": 0,
        "dedicated_cores": []
      }},
      "logging": {{
        "log_level": "info",
        "path": "{}",
        "format": "%(log_level) %(logger) %(message)"
      }}
    }}
  )custom_delimiter";

  std::string jsonConfig = std::vformat(constJsonConfigFormat, std::make_format_args(log_file_path_string));
  std::ofstream(config_file.path()) << jsonConfig;

  char* argv[] = {nullptr, config_file_path_string.data()};

  EXPECT_ANY_THROW(read_config_and_initialize_logging(std::span(argv, argv + 2)));
}

TEST(Run, IncorrectLogLevel) {
  TemporaryDirectory directory("run");
  TemporaryFile logs_file(directory, "logs");
  TemporaryFile config_file(directory, "config");

  auto config_file_path_string = config_file.path().string();
  auto log_file_path_string = logs_file.path().string();

  static constexpr std::string_view constJsonConfigFormat = R"custom_delimiter(
    {{
      "cpu_cores": {{
        "shared_core": 0,
        "dedicated_cores": []
      }},
      "logging": {{
        "type": "file",
        "log_level": "info1",
        "path": "{}",
        "format": "%(log_level) %(logger) %(message)"
      }}
    }}
  )custom_delimiter";

  std::string jsonConfig = std::vformat(constJsonConfigFormat, std::make_format_args(log_file_path_string));
  std::ofstream(config_file.path()) << jsonConfig;

  char* argv[] = {nullptr, config_file_path_string.data()};

  EXPECT_ANY_THROW(read_config_and_initialize_logging(std::span(argv, argv + 2)));
}


TEST(Run, BasicTest) {
  TemporaryDirectory directory("run");
  TemporaryFile logs_file(directory, "logs");
  TemporaryFile config_file(directory, "config");

  auto config_file_path_string = config_file.path().string();
  auto log_file_path_string = logs_file.path().string();

  static constexpr std::string_view constJsonConfigFormat = R"custom_delimiter(
    {{
      "cpu_cores": {{
        "shared_core": 0,
        "dedicated_cores": []
      }},
      "logging": {{
        "type": "file",
        "log_level": "info",
        "path": "{}",
        "format": "%(log_level) %(logger) %(message)"
      }}
    }}
  )custom_delimiter";

  std::string jsonConfig = std::vformat(constJsonConfigFormat, std::make_format_args(log_file_path_string));
  std::ofstream(config_file.path()) << jsonConfig;

  char* argv[] = {nullptr, config_file_path_string.data()};

  auto [runtime, config] = read_config_and_initialize_logging(std::span(argv, argv + 2));
  ASSERT_TRUE(config.contains("logging"));
  EXPECT_EQ(config["logging"]["type"].as<std::string>(), "file");
  EXPECT_EQ(config["logging"]["log_level"].as<std::string>(), "info");
  EXPECT_EQ(config["logging"]["format"].as<std::string>(), "%(log_level) %(logger) %(message)");
  EXPECT_EQ(config["logging"]["path"].as<std::string>(), log_file_path_string);
  LoggingBackend::instance().get_logger()->flush_log();

  std::istringstream stream(read_file(logs_file.path()));
  std::vector<std::string> lines;
  for (std::string line; std::getline(stream, line);) {
    lines.emplace_back(std::move(line));
  }

  EXPECT_EQ(lines.size(), 3);
  EXPECT_EQ(lines[0], "INFO root Initialized logging backend");

  static constexpr std::string_view constLine1Format =
      R"custom_delimiter(INFO root config={{"cpu_cores":{{"shared_core":0,"dedicated_cores":[]}},"logging":{{"type":"file","log_level":"info","path":"{}","format":"%(log_level) %(logger) %(message)"}}}})custom_delimiter";

  EXPECT_EQ(lines[1], std::vformat(constLine1Format, std::make_format_args(log_file_path_string)));

  static constexpr std::string_view constLine2Prefix = "INFO root Initialized time backend, tsc_hw: ";
  ASSERT_TRUE(lines[2].starts_with(constLine2Prefix));
}