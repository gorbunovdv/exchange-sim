#include "base/config/config.h"
#include "base/temporary.h"
#include <gtest/gtest.h>
#include <string_view>
#include <fstream>

TEST(ConfigTest, MissingConfigFile) {
  using namespace exchange_sim;

  TemporaryDirectory directory("config");
  std::filesystem::path path = TemporaryFile(directory, "config").path();

  EXPECT_THROW(load_config_from_file(path), std::runtime_error);
}

TEST(ConfigTest, InvalidJson) {
  using namespace exchange_sim;

  static constexpr std::string_view constDefaultConfig = R"custom_delimiter(
    {
      "logging": {
        "type": "console",
        "log_level": "info",
        "format: "%(time) [%(thread_id)] %(short_source_location:) %(log_level) %(logger) %(message)"
      }
    }
  )custom_delimiter";

  TemporaryDirectory directory("config");
  TemporaryFile file(directory, "config");
  std::ofstream(file.path()) << constDefaultConfig;

  EXPECT_ANY_THROW(load_config_from_file(file.path()));
}

TEST(ConfigTest, BasicConfigTest) {
  static constexpr std::string_view constDefaultConfig = R"custom_delimiter(
    {
      "logging": {
        "type": "console",
        "log_level": "info",
        "format": "%(time) [%(thread_id)] %(short_source_location:) %(log_level) %(logger) %(message)"
      }
    }
  )custom_delimiter";

  using namespace exchange_sim;

  TemporaryDirectory directory("config");
  TemporaryFile file(directory, "config");
  std::ofstream(file.path()) << constDefaultConfig;

  auto config = load_config_from_file(file.path());
  EXPECT_EQ(config["logging"]["type"].as<std::string>(), "console");
  EXPECT_EQ(config["logging"]["log_level"].as<std::string>(), "info");
  EXPECT_EQ(config["logging"]["format"].as<std::string>(),
      "%(time) [%(thread_id)] %(short_source_location:) %(log_level) %(logger) %(message)");
}