#include <gtest/gtest.h>

#include "base/time/rdtsc.h"

using namespace exchange_sim;

TEST(RdtscTest, SleepTimeTest) {
  static constexpr std::chrono::milliseconds constTimeToSleep = 1s;
  static constexpr std::chrono::milliseconds constTimeFaultTolerance = 20ms;

  rdtsc_frequency();

  auto time1 = Ticks::rdtscp();
  std::this_thread::sleep_for(constTimeToSleep);
  auto time2 = Ticks::rdtscp();

  EXPECT_NEAR(std::chrono::duration_cast<std::chrono::milliseconds>(time2.nanoseconds_distance(time1)).count(),
      constTimeToSleep.count(),
      constTimeFaultTolerance.count());
}
