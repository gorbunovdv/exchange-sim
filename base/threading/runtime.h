#pragma once
#include "base/threading/core_allocator.h"
#include "base/threading/signals.h"

namespace exchange_sim {

class Runtime : NonMoveable {
public:
  struct Config {
    CoreAllocator::Config core_allocator_config;
  };

  explicit Runtime(const Config& config);

  CoreAllocator& core_allocator() { return core_allocator_; }

  std::stop_token stop_token() const { return stop_source_.get_token(); }

private:
  CoreAllocator core_allocator_;
  std::stop_source stop_source_;
  ShutdownSignalThread shutdown_signal_;
};

}  // namespace exchange_sim