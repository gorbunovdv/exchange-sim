#include "logging_backend.h"

#include <iostream>

namespace exchange_sim {

const LoggingBackend::LoggingConfig LoggingBackend::constDefaultConfig = {
  .type = LoggingConfig::Type::CONSOLE,
  .path = std::nullopt,
  .log_level = quill::LogLevel::Info,
  .format = quill::PatternFormatterOptions()
};

LoggingBackend& LoggingBackend::get_or_create(const LoggingConfig* config) {
  static std::unique_ptr<LoggingBackend> logging_backend = nullptr;
  if (logging_backend == nullptr) {
    quill::Backend::start();
    auto sink_ = [](const LoggingConfig* config) -> std::shared_ptr<quill::Sink> {
      if (config->type == LoggingConfig::Type::FILE) {
        return quill::Frontend::create_or_get_sink<quill::FileSink>(config->path.value());
      }
      if (config->type == LoggingConfig::Type::CONSOLE) {
        return quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console");
      }
      std::cerr << "Unknown log type: " << magic_enum::enum_name(config->type) << std::endl;
      std::exit(1);
    }(config);
    logging_backend = std::unique_ptr<LoggingBackend>{new LoggingBackend(*config, std::move(sink_))};
  }
  return *logging_backend;
}

}