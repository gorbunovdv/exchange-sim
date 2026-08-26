#pragma once

#include "base/time/rdtsc.h"

namespace exchange_sim {

template<typename Object>
struct Timestamped {
  Object object;
  Ticks timestamp;
};

}  // namespace exchange_sim
