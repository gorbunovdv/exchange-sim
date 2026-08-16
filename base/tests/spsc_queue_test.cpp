#include "base/spsc_queue.h"
#include "base/time/rdtsc.h"

#include <gtest/gtest.h>

using namespace exchange_sim;

static constexpr int constIterCount = 100000;

TEST(SPSCQueue, BasicQueueTest) {
  SPSCQueue<int> queue(3);
  ASSERT_EQ(queue.capacity(), 2);
  ASSERT_TRUE(queue.try_push(1));
  queue.push(2);
  ASSERT_EQ(queue.size(), 2);
  {
    int result;
    ASSERT_TRUE(queue.try_pop(result));
    EXPECT_EQ(result, 1);
  }
  ASSERT_FALSE(queue.empty());
  ASSERT_EQ(queue.size(), 1);
  {
    int result;
    ASSERT_TRUE(queue.try_pop(result));
    EXPECT_EQ(result, 2);
  }
  ASSERT_TRUE(queue.empty());
  queue.produce([](int& holder) { holder = 3; });
  queue.consume([](int& holder) { EXPECT_EQ(holder, 3); });
  EXPECT_EQ(queue.consume_span([](std::span<int> sp) { return sp.size(); }), 0);
  queue.produce([](int& holder) { holder = 4; });
  queue.produce([](int& holder) { holder = 5; });
  EXPECT_EQ(queue.consume_span([](std::span<int> sp) { return sp.size(); }), 2);
}

TEST(SPSCQueue, EmplaceTest) {
  SPSCQueue<std::pair<int, int>> queue(3);
  queue.emplace(1, 2);
  queue.emplace(3, 4);
  queue.consume_span([&](std::span<std::pair<int, int>> sp) {
    EXPECT_EQ(sp[0], (std::pair{1, 2}));
    EXPECT_EQ(sp[1], (std::pair{3, 4}));
    return true;
  });
}

TEST(SPSCQueue, AlmostEmptyTest) {
  SPSCQueue<int> queue(constIterCount / 100);
  {
    std::jthread consumer([&queue] {
      for (int i = 0; i < constIterCount; i++) {
        while (!queue.consume([&](const int& value) { EXPECT_EQ(value, i); })) {
          std::this_thread::yield();
        }
      }
    });
    std::jthread producer([&queue] {
      for (int i = 0; i < constIterCount; i++) {
        while (!queue.produce([&](int& value) { value = i; })) {
        }
        std::this_thread::sleep_for(1us);
      }
    });
  }
  ASSERT_TRUE(queue.empty());
}

TEST(SPSCQueue, AlmostFullTest) {
  SPSCQueue<int> queue(constIterCount / 100);
  {
    std::jthread consumer([&queue] {
      for (int i = 0; i < constIterCount; i++) {
        while (!queue.consume([&](const int& value) { EXPECT_EQ(value, i); })) {
        }
        std::this_thread::sleep_for(1us);
      }
    });
    std::jthread producer([&queue] {
      for (int i = 0; i < constIterCount; i++) {
        while (!queue.produce([&](int& value) { value = i; })) {
          std::this_thread::yield();
        }
      }
    });
  }
  ASSERT_TRUE(queue.empty());
}

TEST(SPSCQueue, HalfFullTest) {
  SPSCQueue<int> queue(constIterCount / 100);
  for (size_t i = 0; i < queue.capacity() / 2; i++) {
    queue.push(static_cast<int>(i));
  }
  {
    std::jthread consumer([&queue] {
      for (int i = 0; i < constIterCount; i++) {
        while (!queue.consume([&](const int& value) { EXPECT_EQ(value, i); })) {
          std::this_thread::yield();
        }
      }
    });
    std::jthread producer([&queue] {
      for (int i = static_cast<int>(queue.capacity() / 2); i < constIterCount; i++) {
        while (!queue.produce([&](int& value) { value = i; })) {
          std::this_thread::yield();
        }
      }
    });
  }
  ASSERT_TRUE(queue.empty());
}