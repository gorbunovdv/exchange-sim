#pragma once

#include <cstddef>

namespace exchange_sim {

inline constexpr size_t constCacheLineSize = 64;
inline constexpr size_t constCacheLineSeparationSize = constCacheLineSize * 2;

}  // namespace exchange_sim
