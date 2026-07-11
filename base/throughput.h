#pragma once
#include "base/time/rdtsc.h"

#include "logging/loggable_class.h"

#include <string_view>

namespace exchange_sim {

class Throughput : public LoggableClass {
public:
  explicit Throughput(std::string_view name): LoggableClass(std::string(name) + "_throughput") {}

  void tick(uint32_t count = 1) { tick(count, Ticks::rdtscp()); }

  void tick(Ticks current) { tick(1, current); }

  void tick(uint32_t count, Ticks current) {
    counter_ += count;
    if (current.nanoseconds_distance(prev_tick_) > 10s) {
      uint32_t events_processed = counter_;
      double nanoseconds_per_event = current.nanoseconds_distance(prev_tick_).count() / events_processed;
      double events_per_second = (1s / 1ns) / nanoseconds_per_event;
      INFO("Calculating throughput, events_processed={} time_elapsed_ns={} ns/event={} events/s={:.2f}",
          events_processed,
          current.nanoseconds_distance(prev_tick_).count(),
          nanoseconds_per_event,
          events_per_second,
          events_per_second);
      counter_ = 0;
      prev_tick_ = current;
    }
  }

private:
  uint32_t counter_ = 0;
  Ticks prev_tick_ = Ticks{0};
};

}  // namespace exchange_sim