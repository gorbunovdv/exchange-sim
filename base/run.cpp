#include "run.h"

#include "base/config/config.h"
#include "base/logging/logging_backend.h"
#include <iostream>

namespace exchange_sim {

glz::generic read_config_and_initialize_logging(std::span<char*> args) {
  auto value_or_raise = [](std::string_view type, auto expected) {
    if (!expected.has_value()) {
      throw std::runtime_error(std::format("{} has no correct value", type));
    }
    return expected.value();
  };

  if (args.size() != 2) {
    std::cout << "usage: " << args[0] << " <config_file>" << '\n';
    throw std::runtime_error("wrong number of arguments");
  }
  glz::generic config = load_config_from_file(args[1]);
  LoggingBackend::LoggingConfig logging_config{
      .type = value_or_raise("logging.type",
          magic_enum::enum_cast<LoggingBackend::LoggingConfig::Type>(
              config["logging"]["type"].as<std::string_view>(), magic_enum::case_insensitive)),
      .path = config["logging"].contains("path") ? std::optional{config["logging"]["path"].as<std::string>()} :
                                                   std::nullopt,
      .log_level = value_or_raise("logging.log_level",
          magic_enum::enum_cast<quill::LogLevel>(
              config["logging"]["log_level"].as<std::string>(), magic_enum::case_insensitive)),
      .format = quill::PatternFormatterOptions(config["logging"]["format"].as<std::string>()),
  };
  LoggingBackend::init(logging_config);
  INFO("Initialized logging backend");
  INFO("config={}", glz::write_json(config).value());
  return config;
}

}  // namespace exchange_sim