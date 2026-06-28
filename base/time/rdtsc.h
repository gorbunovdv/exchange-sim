#pragma once
#include <x86intrin.h>
#include <cstdint>
#include <chrono>

namespace exchange_sim {

using namespace std::chrono_literals;

const uint64_t& rdtsc_frequency();

class Ticks {
public:
  explicit Ticks(uint64_t ticks = 0): ticks_(ticks) {}

  static Ticks rdtsc() noexcept { return Ticks(__rdtsc()); }

  static Ticks rdtscp() noexcept {
    uint32_t aux;
    return Ticks(__rdtscp(&aux));
  }

  Ticks ticks_distance(Ticks other) const { return Ticks(ticks_ - other.ticks_); }

  Ticks ticks_till_now() const { return rdtsc().ticks_distance(*this); }

  std::chrono::nanoseconds nanoseconds_distance(Ticks other) const {
    return std::chrono::nanoseconds(ticks_distance(other).ticks() * 1000 / rdtsc_frequency());
  }

  std::chrono::nanoseconds nanoseconds_till_now() const { return rdtsc().nanoseconds_distance(*this); }

  uint64_t ticks() const { return ticks_; }

  friend Ticks operator+(const Ticks& lhs, const Ticks& rhs) { return Ticks(lhs.ticks_ + rhs.ticks_); }

  friend Ticks operator-(const Ticks& lhs, const Ticks& rhs) { return Ticks(lhs.ticks_ - rhs.ticks_); }

  friend Ticks operator+(const Ticks& lhs, const std::chrono::nanoseconds& rhs) {
    return Ticks(lhs.ticks_ + rhs.count() * rdtsc_frequency() / 1000);
  }

  friend Ticks operator-(const Ticks& lhs, const std::chrono::nanoseconds& rhs) {
    return Ticks(lhs.ticks_ - rhs.count() * rdtsc_frequency() / 1000);
  }

private:
  uint64_t ticks_;
};

}  // namespace exchange_sim