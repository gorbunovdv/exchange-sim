#pragma once

#include "base/logging/loggable_class.h"
#include "base/logging/logging_backend.h"

#include <cpptrace/cpptrace.hpp>

namespace exchange_sim {

class AllocatorTracking {
public:
  struct Config {
    bool enabled_printing = false;
  };

  explicit AllocatorTracking(const Config& config): AllocatorTracking(config.enabled_printing) {}

  explicit AllocatorTracking(bool enabled_printing): enabled_printing_(enabled_printing) {}

  void enable_tracking() { enabled_tracking_ = true; }

  void disable_tracking() {
    enabled_tracking_ = false;
    CHECK(allocated_memory_size_ == 0,
        "AllocatorTracking: total allocated memory size: {}",
        allocated_memory_size_.load());
  }

  static void init(const Config& config) { get_or_create(&config); }

  static AllocatorTracking& instance() { return get_or_create(); }

  void on_new(size_t size) {
    thread_local bool thread_enabled_tracking = true;
    if (enabled_tracking_ && thread_enabled_tracking) {
      thread_enabled_tracking = false;
      allocated_memory_size_ += size;
      if (enabled_printing_) {
        INFO("Allocated {} bytes, trace={}", size, cpptrace::generate_trace().to_string());
      }
      thread_enabled_tracking = true;
    }
  }

  static const Config constDefaultConfig;

private:
  static AllocatorTracking& get_or_create(const Config* config = &constDefaultConfig) {
    static AllocatorTracking instance(*config);
    return instance;
  }

  std::atomic<size_t> allocated_memory_size_ = 0;
  std::atomic<bool> enabled_tracking_ = false;

  bool enabled_printing_ = false;
};

}  // namespace exchange_sim
