#pragma once

#include <cstdlib>

namespace exchange_sim {

template<typename T, typename Arena>
class CustomAllocator {
public:
  using value_type = T;

  explicit CustomAllocator(Arena* arena): arena_(arena) {}

  T* allocate(size_t n) { return reinterpret_cast<T*>(arena_->allocate(n * sizeof(T))); }

  void deallocate(T* p, size_t n) { arena_->deallocate(p, n * sizeof(T)); }

  template<typename Other>
  CustomAllocator(const CustomAllocator<Other, Arena>& other) noexcept: arena_(other.arena_) {}

private:
  template<typename OtherT, typename OtherArena>
  friend class CustomAllocator;

  Arena* arena_;
};

}  // namespace exchange_sim