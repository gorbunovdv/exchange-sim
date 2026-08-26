#include "base/threading/core_allocator.h"

#include <gtest/gtest.h>

using namespace exchange_sim;

TEST(CoreAllocator, EmptyAllocatorTest) {
  CoreAllocator allocator(false, CpuID{0}, {});
  EXPECT_ANY_THROW(allocator.lease_core(CoreType::DEDICATED));
}

TEST(CoreAllocator, ReturnCoreBackToAllocator) {
  CoreAllocator allocator(false, CpuID{0}, {CpuID{1}});
  {
    auto lease = allocator.lease_core(CoreType::DEDICATED);
    EXPECT_EQ(lease.type(), CoreType::DEDICATED);
    auto lease2 = std::move(lease);
    EXPECT_EQ(lease2.type(), CoreType::DEDICATED);
    EXPECT_ANY_THROW(allocator.lease_core(CoreType::DEDICATED));
  }
  EXPECT_EQ(allocator.lease_core(CoreType::DEDICATED).type(), CoreType::DEDICATED);
}

TEST(CoreAllocator, CorePinningDisabled) {
  CoreAllocator allocator(true, CpuID{0}, {CpuID{1}});
  std::vector<CoreLease> leases;
  leases.reserve(10);
  for (size_t i = 0; i < 10; i++) {
    leases.emplace_back(allocator.lease_core(CoreType::DEDICATED));
    leases.emplace_back(allocator.lease_core(CoreType::SHARED));
  }
  for (CoreLease& lease : leases) {
    lease.pin_thread("test");
  }
  leases.clear();
}