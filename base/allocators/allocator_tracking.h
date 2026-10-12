#pragma once

#include "base/logging/loggable_class.h"
#include "base/logging/logging_backend.h"

#include <cpptrace/cpptrace.hpp>

namespace exchange_sim {

class AllocatorTracking {
public:
  void enable_tracking() { enabled_tracking_ = true; }

  void disable_tracking();

  static AllocatorTracking& instance() {
    thread_local AllocatorTracking instance;
    return instance;
  }

  void on_new(size_t size) noexcept;

private:
  size_t allocated_memory_size_ = 0;
  bool enabled_tracking_ = false;

  std::optional<std::exception_ptr> last_exception_;
};

}  // namespace exchange_sim
