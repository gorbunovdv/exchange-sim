#pragma once

#include "base/logging/loggable_class.h"
#include "base/logging/logging_backend.h"

#include <cpptrace/cpptrace.hpp>

namespace exchange_sim {

class AllocatorTracking {
public:
  void enable_tracking() { enabled_tracking_ = true; }

  void disable_tracking() {
    enabled_tracking_ = false;
    CHECK(allocated_memory_size_ == 0,
        "AllocatorTracking: total allocated memory size: {}",
        allocated_memory_size_.load());
  }

  static AllocatorTracking& instance() {
    static AllocatorTracking instance;
    return instance;
  }

  void on_new(size_t size);

private:
  std::atomic<size_t> allocated_memory_size_ = 0;
  std::atomic<bool> enabled_tracking_ = false;
};

}  // namespace exchange_sim
