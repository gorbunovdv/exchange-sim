#pragma once

#include "base/noncopyable.h"
#include "base/threading/cpu_id.h"

#include <quill/LogMacros.h>
#include <quill/Logger.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace exchange_sim {

class LoggingBackend : NonCopyable, NonMoveable {
public:
  struct LoggingConfig {
    enum class Type : std::uint8_t { FILE, CONSOLE };

    Type type;
    std::optional<std::filesystem::path> path;
    quill::LogLevel log_level;
    quill::PatternFormatterOptions format;
    std::optional<CpuID> cpu_id;
  };

  static void init(const LoggingConfig& config) { get_or_create(&config); }

  static LoggingBackend& instance() { return get_or_create(); }

  quill::Logger* get_logger() { return root_logger_; }

  quill::Logger* create_logger(const std::string& name);

  static const LoggingConfig constDefaultConfig;

private:
  LoggingBackend(LoggingConfig config, std::shared_ptr<quill::Sink> sink);

  static LoggingBackend& get_or_create(const LoggingConfig* config = &constDefaultConfig);

  LoggingConfig config_;
  std::shared_ptr<quill::Sink> sink_;
  quill::Logger* root_logger_;
};

quill::Logger* get_logger();

#define INFO(...) LOG_INFO(get_logger(), __VA_ARGS__)
#define DEBUG(...) LOG_DEBUG(get_logger(), __VA_ARGS__)
#define WARNING(...) LOG_WARNING(get_logger(), __VA_ARGS__)
#define ERROR(...) LOG_ERROR(get_logger(), __VA_ARGS__)
#define CRITICAL(...) \
  do { \
    [&] [[gnu::noinline, gnu::cold, noreturn]] () { \
      LOG_CRITICAL(get_logger(), __VA_ARGS__); \
      throw std::runtime_error("Critical error occured"); \
    }(); \
  } while (false)

#define CHECK(condition, ...) \
  do { \
    if (!(condition)) [[unlikely]] { \
      CRITICAL(__VA_ARGS__); \
    } \
  } while (false)

}  // namespace exchange_sim
