#pragma once
#include "base/logging/logging_backend.h"
#include "base/time/rdtsc.h"

#include <chrono>

namespace exchange_sim {

template<typename Callback>
class GuardedTest {
public:
  GuardedTest(std::chrono::nanoseconds max_latency, auto&& callback):
      max_latency_(max_latency), callback_(std::forward<decltype(callback)>(callback)), start_(Ticks::rdtsc()) {}

  ~GuardedTest() {
    if (auto time_spent = start_.nanoseconds_till_now(); time_spent > max_latency_) {
      ERROR("Time spent exceeded max latency, time_spent_ns: {} max_time_spent: {}",
          time_spent.count(),
          max_latency_.count());
      callback_();
    }
  }

private:
  std::chrono::nanoseconds max_latency_;
  Callback callback_;
  Ticks start_;
};

}  // namespace exchange_sim
