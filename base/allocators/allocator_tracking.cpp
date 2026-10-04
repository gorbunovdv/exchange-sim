#include "allocator_tracking.h"

#if !__has_feature(address_sanitizer) && !defined(__SANITIZE_ADDRESS__) && !__has_feature(thread_sanitizer) && \
    !defined(__SANITIZE_THREAD__)

#include <mimalloc.h>

void operator delete(void* p) noexcept {
  mi_free(p);
};
void operator delete[](void* p) noexcept {
  mi_free(p);
};

void operator delete(void* p, const std::nothrow_t&) noexcept {
  mi_free(p);
}
void operator delete[](void* p, const std::nothrow_t&) noexcept {
  mi_free(p);
}

void* operator new(std::size_t n) noexcept(false) {
  using namespace exchange_sim;

  AllocatorTracking::instance().on_new(n);
  return mi_new(n);
}
void* operator new[](std::size_t n) noexcept(false) {
  using namespace exchange_sim;

  AllocatorTracking::instance().on_new(n);
  return mi_new(n);
}

void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  using namespace exchange_sim;

  AllocatorTracking::instance().on_new(n);
  return mi_new_nothrow(n);
}
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept {
  using namespace exchange_sim;

  AllocatorTracking::instance().on_new(n);
  return mi_new_nothrow(n);
}

void operator delete(void* p, std::size_t n) noexcept {
  mi_free_size(p, n);
};
void operator delete[](void* p, std::size_t n) noexcept {
  mi_free_size(p, n);
};

void operator delete(void* p, std::align_val_t al) noexcept {
  mi_free_aligned(p, static_cast<size_t>(al));
}
void operator delete[](void* p, std::align_val_t al) noexcept {
  mi_free_aligned(p, static_cast<size_t>(al));
}
void operator delete(void* p, std::size_t n, std::align_val_t al) noexcept {
  mi_free_size_aligned(p, n, static_cast<size_t>(al));
};
void operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept {
  mi_free_size_aligned(p, n, static_cast<size_t>(al));
};
void operator delete(void* p, std::align_val_t al, const std::nothrow_t&) noexcept {
  mi_free_aligned(p, static_cast<size_t>(al));
}
void operator delete[](void* p, std::align_val_t al, const std::nothrow_t&) noexcept {
  mi_free_aligned(p, static_cast<size_t>(al));
}

void* operator new(std::size_t n, std::align_val_t al) noexcept(false) {
  using namespace exchange_sim;

  AllocatorTracking::instance().on_new(n);
  return mi_new_aligned(n, static_cast<size_t>(al));
}

void* operator new[](std::size_t n, std::align_val_t al) noexcept(false) {
  using namespace exchange_sim;

  AllocatorTracking::instance().on_new(n);
  return mi_new_aligned(n, static_cast<size_t>(al));
}

void* operator new(std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept {
  using namespace exchange_sim;

  AllocatorTracking::instance().on_new(n);
  return mi_new_aligned_nothrow(n, static_cast<size_t>(al));
}

void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept {
  using namespace exchange_sim;

  AllocatorTracking::instance().on_new(n);
  return mi_new_aligned_nothrow(n, static_cast<size_t>(al));
}

#endif

namespace exchange_sim {

const AllocatorTracking::Config AllocatorTracking::constDefaultConfig = {
    .enabled_printing = false,
};

}  // namespace exchange_sim