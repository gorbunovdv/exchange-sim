#pragma once
#include "base/threading/core_allocator.h"
#include "base/threading/signals.h"

#include <utility>

namespace exchange_sim {

class ShutdownSignal {
public:
  explicit ShutdownSignal(std::stop_source stop_source, CoreLease&& core_lease);

  ~ShutdownSignal();

private:
  std::stop_source stop_source_;
  CoreLease core_lease_;
  std::atomic_bool teardown_{false};
  std::jthread thread_;
};

class Runtime {
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
  ShutdownSignal shutdown_signal_;
};

}  // namespace exchange_sim