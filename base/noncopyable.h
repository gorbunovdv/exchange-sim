#pragma once

namespace exchange_sim {

struct NonCopyable {
  NonCopyable() = default;

  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;

  NonCopyable(NonCopyable&&) = default;
  NonCopyable& operator=(NonCopyable&&) = default;
};

struct NonMoveable {
  NonMoveable() = default;

  NonMoveable(const NonMoveable&) = delete;
  NonMoveable& operator=(const NonMoveable&) = delete;

  NonMoveable(NonMoveable&&) = delete;
  NonMoveable& operator=(NonMoveable&&) = delete;
};

}  // namespace exchange_sim
