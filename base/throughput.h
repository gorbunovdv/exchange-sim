#pragma once
#include "base/time/rdtsc.h"

#include "logging/loggable_class.h"

#include <iostream>
#include <string_view>

namespace exchange_sim {

class Throughput : public LoggableClass {
public:
  explicit Throughput(std::string_view name): LoggableClass(std::string(name) + "_throughput") {}

  ~Throughput() {
    try {
      print_info(Ticks::rdtsc());
    } catch (...) {
      std::cerr << "Failed to print throughput info" << '\n';
    }
  }

  void tick(uint32_t count = 1) { tick(count, Ticks::rdtsc()); }

  void tick(Ticks current) { tick(1, current); }

  void tick(uint32_t count, Ticks current) {
    counter_ += count;
    if (current.nanoseconds_distance(prev_tick_) > 10s) {
      if (prev_tick_ != Ticks{0}) {
        print_info(current);
      }
      counter_ = 0;
      prev_tick_ = current;
    }
  }

private:
  void print_info(Ticks current) {
    uint32_t events_processed = counter_;
    double nanoseconds_per_event = events_processed != 0 ?
        static_cast<double>(current.nanoseconds_distance(prev_tick_).count()) / events_processed :
        std::numeric_limits<double>::infinity();
    double events_per_second = (1s / 1ns) / nanoseconds_per_event;
    INFO("Calculating throughput, events_processed={} time_elapsed_s={} ns/event={} events/s={:.2f}",
        events_processed,
        std::chrono::duration_cast<std::chrono::seconds>(current.nanoseconds_distance(prev_tick_)).count(),
        nanoseconds_per_event,
        events_per_second,
        events_per_second);
  }

  uint32_t counter_ = 0;
  Ticks prev_tick_ = Ticks{0};
};

}  // namespace exchange_sim