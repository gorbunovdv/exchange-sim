#pragma once

#include "base/logging/logging_backend.h"

#include "logging/loggable_class.h"

#include <cstdlib>

namespace exchange_sim {

class AllocatorTracking {
public:
  void enable_tracking() { enabled_tracking_ = true; }

  void disable_tracking() {
    enabled_tracking_ = false;
    if (allocated_memory_size_ > 0) {
      INFO("AllocatorTracking: total allocated memory size: {}", allocated_memory_size_);
    }
  }

  static AllocatorTracking& instance() {
    static AllocatorTracking instance;
    return instance;
  }

  void on_new(size_t size) {
    if (enabled_tracking_) {
      allocated_memory_size_ += size;
      if (print_allocations_) {
        INFO("AllocatorTracking: allocated memory size: {}", allocated_memory_size_);
      }
    }
  }

private:
  size_t allocated_memory_size_ = 0;
  std::atomic<bool> enabled_tracking_ = false;

  bool print_allocations_ = false;
};

}  // namespace exchange_sim
