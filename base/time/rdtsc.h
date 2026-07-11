#pragma once
#include <chrono>
#include <cstdint>

#if defined(__x86_64__)
#include <x86intrin.h>
#elif defined(__aarch64__)
inline uint64_t __rdtsc() {
  uint64_t val;
  asm volatile("mrs %0, cntvct_el0" : "=r"(val));
  return val;
}

inline uint64_t __rdtscp(unsigned int* aux) {
  asm volatile("isb" : : : "memory");
  uint64_t val;
  asm volatile("mrs %0, cntvct_el0" : "=r"(val));
  if (aux) {
    *aux = 0;
  }
  return val;
}
#else
#error "Architecture not supported"
#endif

namespace exchange_sim {

using namespace std::chrono_literals;

const uint64_t& rdtsc_frequency();

class Ticks {
public:
  constexpr explicit Ticks(uint64_t ticks = 0): ticks_(ticks) {}

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

  bool operator==(const Ticks& other) const = default;

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
