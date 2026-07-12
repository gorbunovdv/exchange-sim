#pragma once

#include "base/threading/core_allocator.h"

#include <signal.h>
#include <stop_token>

namespace exchange_sim {

constexpr int constWakeupSignal = SIGUSR1;

sigset_t shutdown_signal_set();

void block_shutdown_signals();

class ShutdownSignalThread {
public:
  explicit ShutdownSignalThread(std::stop_source stop_source, CoreLease&& core_lease);

  ~ShutdownSignalThread();

private:
  std::stop_source stop_source_;
  CoreLease core_lease_;
  std::atomic_bool teardown_{false};
  std::jthread thread_;
};

}  // namespace exchange_sim