#pragma once

namespace exchange_sim {

template<typename T, typename... Args>
constexpr bool equal_to_one_of(const T& key, const Args&&... args) {
  return ((key == args) || ...);
}

}  // namespace exchange_sim
