#include "base/config/config.h"
#include "base/temporary.h"
#include <gtest/gtest.h>
#include <string_view>
#include <fstream>

using namespace exchange_sim;

TEST(ConfigTest, MissingConfigFile) {
  TemporaryDirectory directory("config");
  std::filesystem::path path = TemporaryFile(directory, "config").path();

  EXPECT_THROW(load_config_from_file(path), std::runtime_error);
}

TEST(ConfigTest, InvalidJson) {
  static constexpr std::string_view constDefaultConfig = R"custom_delimiter(
    {
      "logging": {
        "type": "console",
        "log_level": "info",
        "format /* here's no closing quote */ : "%(time) [%(thread_id)] %(short_source_location:) %(log_level) %(logger) %(message)"
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
        "format" /* there's quote in correct location */: "%(time) [%(thread_id)] %(short_source_location:) %(log_level) %(logger) %(message)"
      }
    }
  )custom_delimiter";
  TemporaryDirectory directory("config");
  TemporaryFile file(directory, "config");
  std::ofstream(file.path()) << constDefaultConfig;

  auto config = load_config_from_file(file.path());
  ASSERT_TRUE(config.contains("logging"));
  EXPECT_EQ(config["logging"]["type"].as<std::string>(), "console");
  EXPECT_EQ(config["logging"]["log_level"].as<std::string>(), "info");
  EXPECT_EQ(config["logging"]["format"].as<std::string>(),
      "%(time) [%(thread_id)] %(short_source_location:) %(log_level) %(logger) %(message)");
}

TEST(ConfigTest, DefaultConfigTest) {
  std::filesystem::path default_config_path = EXCHANGE_SIM_DEFAULT_CONFIG_PATH;

  ASSERT_TRUE(std::filesystem::exists(default_config_path)) << default_config_path;

  auto config = load_config_from_file(default_config_path);

  ASSERT_TRUE(config.contains("logging"));
  EXPECT_EQ(config["logging"]["type"].as<std::string>(), "console");
  EXPECT_EQ(config["logging"]["log_level"].as<std::string>(), "info");
  EXPECT_EQ(config["logging"]["format"].as<std::string>(),
      "%(time) [%(thread_id)] %(short_source_location) %(log_level) %(logger) %(message)");
}