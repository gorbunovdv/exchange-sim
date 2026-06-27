#pragma once

namespace exchange_sim {

struct NonCopyable {
  NonCopyable() = default;

  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
};

struct NonMoveable {
  NonMoveable() = default;

  NonMoveable(NonMoveable&&) = delete;
  NonMoveable& operator=(NonMoveable&&) = delete;
};

}  // namespace exchange_sim