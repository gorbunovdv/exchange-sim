#include "logging_backend.h"
#include <iostream>

namespace exchange_sim {

quill::Logger* LoggingBackend::create_logger(const std::string& name) {
  quill::Logger* logger = quill::Frontend::create_or_get_logger(name, sink_, config_.format);
  logger->set_log_level(config_.log_level);
  return logger;
}

const LoggingBackend::LoggingConfig LoggingBackend::constDefaultConfig = {
    .type = LoggingConfig::Type::CONSOLE,
    .path = std::nullopt,
    .log_level = quill::LogLevel::Info,
    .format = quill::PatternFormatterOptions(),
};

quill::Logger* get_logger() {
  return LoggingBackend::instance().get_logger();
}

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
      throw std::runtime_error(std::format("unknown log type, type={}", magic_enum::enum_name(config->type)));
    }(config);
    logging_backend = std::unique_ptr<LoggingBackend>{new LoggingBackend(*config, std::move(sink_))};
  }
  return *logging_backend;
}

}  // namespace exchange_sim