#pragma once
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/LogMacros.h>
#include <magic_enum/magic_enum.hpp>

namespace exchange_sim {

class LoggingBackend {
public:
  struct LoggingConfig {
    enum class Type {
      FILE,
      CONSOLE
    };

    Type type;
    std::optional<std::filesystem::path> path;
    quill::LogLevel log_level;
    quill::PatternFormatterOptions format;
  };

  ~LoggingBackend() {
    root_logger_->flush_log();
  }

  static void init(const LoggingConfig& config) {
    get_or_create(&config);
  }

  static LoggingBackend& get_or_create(const LoggingConfig* config = &constDefaultConfig);

  static LoggingBackend& instance() {
    return get_or_create();
  }

  quill::LogLevel get_log_level() const {
    return config_.log_level;
  }

  quill::PatternFormatterOptions get_format() const {
    return config_.format;
  }

  quill::Logger* get_logger() {
    return root_logger_;
  }

  std::shared_ptr<quill::Sink> get_sink() {
    return sink_;
  }

  static const LoggingConfig constDefaultConfig;

private:
  LoggingBackend(LoggingConfig config, std::shared_ptr<quill::Sink> sink):
    config_(std::move(config)),
    sink_(std::move(sink)),
    root_logger_(quill::Frontend::create_or_get_logger("root", sink_, config_.format)) {
    root_logger_->set_log_level(config_.log_level);
  }

  LoggingConfig config_;
  std::shared_ptr<quill::Sink> sink_;
  quill::Logger* root_logger_;
};

inline quill::Logger* get_logger() {
  return LoggingBackend::instance().get_logger();
}

#define INFO(...) LOG_INFO(get_logger(), __VA_ARGS__)
#define DEBUG(...) LOG_DEBUG(get_logger(), __VA_ARGS__)
#define ERROR(...) LOG_ERROR(get_logger(), __VA_ARGS__)
#define CRITICAL(...) LOG_CRITICAL(get_logger(), __VA_ARGS__)

}