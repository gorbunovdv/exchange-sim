#pragma once
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <magic_enum/magic_enum.hpp>

namespace exchange_sim {

class LoggingBackend {
public:
  struct LoggingConfig {
    enum class Type : std::uint8_t { FILE, CONSOLE };

    Type type;
    std::optional<std::filesystem::path> path;
    quill::LogLevel log_level;
    quill::PatternFormatterOptions format;
  };

  static void init(const LoggingConfig& config) { get_or_create(&config); }

  static LoggingBackend& get_or_create(const LoggingConfig* config = &constDefaultConfig);

  static LoggingBackend& instance() { return get_or_create(); }

  quill::Logger* get_logger() { return root_logger_; }

  quill::Logger* create_logger(const std::string& name);

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
#define CRITICAL(...) \
  do { \
    LOG_CRITICAL(get_logger(), __VA_ARGS__); \
    std::abort(); \
  } while (false)

}  // namespace exchange_sim
