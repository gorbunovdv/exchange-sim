#include "logging_backend.h"

#include "base/logging/quill_sinks.h"

#include <magic_enum/magic_enum.hpp>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>

#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

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
    .cpu_id = CpuID{0},
};

LoggingBackend::LoggingBackend(LoggingConfig config, std::shared_ptr<quill::Sink> sink):
    config_(std::move(config)),
    sink_(std::move(sink)),
    root_logger_(quill::Frontend::create_or_get_logger("root", sink_, config_.format)) {
  root_logger_->set_log_level(config_.log_level);
}

quill::Logger* get_logger() {
  return LoggingBackend::instance().get_logger();
}

LoggingBackend& LoggingBackend::get_or_create(const LoggingConfig* config) {
  static std::unique_ptr<LoggingBackend> logging_backend = [](const LoggingConfig* config) {
    quill::Backend::start(quill::BackendOptions{.cpu_affinity = static_cast<uint16_t>(config->cpu_id.value())});
    auto sink = [](const LoggingConfig* config) -> std::shared_ptr<quill::Sink> {
      if (config->type == LoggingConfig::Type::FILE) {
        if (!config->path.has_value()) {
          throw std::runtime_error("path is required for file logging");
        }
        return quill::Frontend::create_or_get_sink<quill::FileSink>(config->path.value());
      }
      if (config->type == LoggingConfig::Type::CONSOLE) {
        return quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console");
      }
      throw std::runtime_error(std::format("unknown log type, type={}", magic_enum::enum_name(config->type)));
    }(config);
    return std::unique_ptr<LoggingBackend>{new LoggingBackend(*config, std::move(sink))};
  }(config);
  return *logging_backend;
}

}  // namespace exchange_sim
