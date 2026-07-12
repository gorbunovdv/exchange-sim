#pragma once

#include <format>
#include <signal.h>

namespace exchange_sim {

inline constexpr int constWakeupSignal = SIGUSR1;

inline sigset_t shutdown_signal_set() {
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGINT);
  sigaddset(&signal_set, SIGTERM);
  sigaddset(&signal_set, SIGHUP);
  sigaddset(&signal_set, constWakeupSignal);
  return signal_set;
}

inline void block_shutdown_signals() {
  sigset_t signal_set = shutdown_signal_set();
  if (int rc = pthread_sigmask(SIG_BLOCK, &signal_set, nullptr); rc != 0) {
    throw std::runtime_error(std::format("pthread_sigmask failed, rc={}", rc));
  }
}

}  // namespace exchange_sim