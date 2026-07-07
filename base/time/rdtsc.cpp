#include "rdtsc.h"

#include <algorithm>
#include <ranges>

namespace exchange_sim {

const uint64_t& rdtsc_frequency() {
  static uint64_t rdtsc_freq = []() {
    static constexpr size_t constSamplesCount = 11;
    static constexpr std::chrono::nanoseconds constSampleDuration = 10ms;

    std::array<uint64_t, constSamplesCount> samples;
    for (size_t i = 0; i < constSamplesCount; ++i) {
      auto time1 = std::chrono::high_resolution_clock::now();
      uint64_t sample1 = __rdtsc();
      while (std::chrono::high_resolution_clock::now() - time1 < constSampleDuration) {
      }
      auto time2 = std::chrono::high_resolution_clock::now();
      uint64_t sample2 = __rdtsc();
      samples[i] = ((sample2 - sample1) / std::chrono::duration_cast<std::chrono::microseconds>(time2 - time1).count());
    }
    std::ranges::nth_element(samples, samples.begin() + samples.size() / 2);
    return *(samples.begin() + samples.size() / 2);
  }();
  return rdtsc_freq;
}

}  // namespace exchange_sim