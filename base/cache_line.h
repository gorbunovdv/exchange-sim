#pragma once

namespace exchange_sim {
#ifdef __cpp_lib_hardware_interference_size
inline constexpr size_t constCacheLineSize = std::hardware_destructive_interference_size;
#else
inline constexpr size_t constCacheLineSize = 64;
#endif
}  // namespace exchange_sim