#include "base/allocators/allocator_tracking.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <new>
#include <stdexcept>

using namespace exchange_sim;

TEST(AllocatorTrackingTest, BasicNoAllocations) {
  void* allocation = ::operator new(sizeof(int));
  ::operator delete(allocation);

  AllocatorTracking::instance().enable_tracking();
  AllocatorTracking::instance().disable_tracking();
}

TEST(AllocatorTrackingTest, ThrowOneAllocation) {
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__) || __has_feature(thread_sanitizer) || \
    defined(__SANITIZE_THREAD__)
  GTEST_SKIP() << "Allocation tracking is disabled under ASan/TSan";
#else
  AllocatorTracking::instance().enable_tracking();
  void* allocation = ::operator new(sizeof(int));
  ::operator delete(allocation);
  EXPECT_THROW(AllocatorTracking::instance().disable_tracking(), std::runtime_error);
#endif
}
