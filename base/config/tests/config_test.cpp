#include "base/config/config.h"
#include "base/temporary.h"
#include <gtest/gtest.h>

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

  TemporaryFile file("config");
  std::ofstream(file.path()) << constDefaultConfig;

  auto config = load_config_from_file(file.path());
  EXPECT_EQ(config["logging"]["type"].as<std::string>(), "console");
  EXPECT_EQ(config["logging"]["log_level"].as<std::string>(), "info");
  EXPECT_EQ(config["logging"]["format"].as<std::string>(),
      "%(time) [%(thread_id)] %(short_source_location:) %(log_level) %(logger) %(message)");
}