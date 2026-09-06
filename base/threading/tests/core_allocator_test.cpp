#include "base/threading/core_allocator.h"

#include <gtest/gtest.h>

using namespace exchange_sim;

TEST(CoreAllocator, EmptyAllocatorTest) {
  CoreAllocator allocator(false, CpuID{0}, {});
  EXPECT_EQ(allocator.lease_core(CoreType::DEDICATED).type(), CoreType::SHARED);
}

TEST(CoreAllocator, ReturnCoreBackToAllocator) {
  CoreAllocator allocator(false, CpuID{0}, {CpuID{1}});
  {
    auto lease = allocator.lease_core(CoreType::DEDICATED);
    EXPECT_EQ(lease.type(), CoreType::DEDICATED);
    auto lease2 = std::move(lease);
    EXPECT_EQ(lease2.type(), CoreType::DEDICATED);
    EXPECT_EQ(allocator.lease_core(CoreType::DEDICATED).type(), CoreType::SHARED);
  }
  EXPECT_EQ(allocator.lease_core(CoreType::DEDICATED).type(), CoreType::DEDICATED);
}

TEST(CoreAllocator, FallbackToNone) {
  CoreAllocator allocator(true, std::nullopt, {});
  EXPECT_EQ(allocator.lease_core(CoreType::DEDICATED).type(), CoreType::NONE);
  EXPECT_EQ(allocator.lease_core(CoreType::SHARED).type(), CoreType::NONE);
  EXPECT_EQ(allocator.lease_core(CoreType::NONE).type(), CoreType::NONE);
}

TEST(CoreAllocator, NoneWithoutCores) {
  CoreAllocator allocator(false, std::nullopt, {});
  EXPECT_EQ(allocator.lease_core(CoreType::NONE).type(), CoreType::NONE);
}