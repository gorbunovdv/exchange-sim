#include "base/mpsc_queue.h"
#include "base/time/rdtsc.h"

#include <gtest/gtest.h>

using namespace exchange_sim;

static constexpr int constIterCount = 100000;
static constexpr int constProducerCount = 4;
static constexpr int constIterPerProducer = constIterCount / constProducerCount;

using Message = std::pair<int, int>;

namespace {

void consume_all(MPSCQueue<Message>& queue, bool yield_on_empty, bool sleep_after_consume) {
  std::vector expected(constProducerCount, 0);
  for (int i = 0; i < constIterCount; i++) {
    while (!queue.consume([&](const Message& message) {
      auto [producer, value] = message;
      ASSERT_GE(producer, 0);
      ASSERT_LT(producer, constProducerCount);
      EXPECT_EQ(value, expected[producer]);
      expected[producer] = value + 1;
    })) {
      if (yield_on_empty) {
        std::this_thread::yield();
      }
    }
    if (sleep_after_consume) {
      std::this_thread::sleep_for(1us);
    }
  }
  for (int producer = 0; producer < constProducerCount; producer++) {
    EXPECT_EQ(expected[producer], constIterPerProducer);
  }
}

}  // namespace

TEST(MPSCQueue, BasicQueueTest) {
  MPSCQueue<int> queue(3);
  ASSERT_TRUE(queue.empty());
  ASSERT_TRUE(queue.try_push(1));
  queue.push(2);
  ASSERT_FALSE(queue.try_push(3));
  ASSERT_FALSE(queue.empty());
  {
    int result;
    ASSERT_TRUE(queue.try_pop(result));
    EXPECT_EQ(result, 1);
  }
  ASSERT_FALSE(queue.empty());
  {
    int result;
    ASSERT_TRUE(queue.try_pop(result));
    EXPECT_EQ(result, 2);
  }
  ASSERT_TRUE(queue.empty());
  {
    int result;
    ASSERT_FALSE(queue.try_pop(result));
  }
  queue.produce([](int& holder) { holder = 4; });
  queue.produce([](int& holder) { holder = 5; });
  queue.consume([](int& holder) { EXPECT_EQ(holder, 4); });
  queue.push(6);
  queue.consume([](int& holder) { EXPECT_EQ(holder, 5); });
  queue.consume([](int& holder) { EXPECT_EQ(holder, 6); });
  ASSERT_TRUE(queue.empty());
}

TEST(MPSCQueue, EmplaceTest) {
  MPSCQueue<Message> queue(3);
  queue.emplace(1, 2);
  queue.emplace(3, 4);
  ASSERT_FALSE(queue.try_emplace(5, 6));
  {
    Message result;
    ASSERT_TRUE(queue.try_pop(result));
    EXPECT_EQ(result, (Message{1, 2}));
  }
  queue.consume([](const Message& holder) { EXPECT_EQ(holder, (Message{3, 4})); });
  ASSERT_TRUE(queue.empty());
}

TEST(MPSCQueue, AlmostEmptyTest) {
  MPSCQueue<Message> queue(constIterCount / 100);
  {
    std::jthread consumer([&queue] { consume_all(queue, true, false); });
    std::vector<std::jthread> producers;
    producers.reserve(constProducerCount);
    for (int producer = 0; producer < constProducerCount; producer++) {
      producers.emplace_back([&queue, producer] {
        for (int i = 0; i < constIterPerProducer; i++) {
          while (!queue.produce([&](Message& message) { message = {producer, i}; })) {
          }
          std::this_thread::sleep_for(1us);
        }
      });
    }
  }
  ASSERT_TRUE(queue.empty());
}

TEST(MPSCQueue, AlmostFullTest) {
  MPSCQueue<Message> queue(constIterCount / 100);
  {
    std::jthread consumer([&queue] { consume_all(queue, false, true); });
    std::vector<std::jthread> producers;
    producers.reserve(constProducerCount);
    for (int producer = 0; producer < constProducerCount; producer++) {
      producers.emplace_back([&queue, producer] {
        for (int i = 0; i < constIterPerProducer; i++) {
          while (!queue.produce([&](Message& message) { message = {producer, i}; })) {
            std::this_thread::yield();
          }
        }
      });
    }
  }
  ASSERT_TRUE(queue.empty());
}

TEST(MPSCQueue, HalfFullTest) {
  static constexpr int constQueueSize = constIterCount / 100;
  static constexpr int constPrefillPerProducer = constQueueSize / 2 / constProducerCount;
  MPSCQueue<Message> queue(constQueueSize);
  for (int i = 0; i < constPrefillPerProducer; i++) {
    for (int producer = 0; producer < constProducerCount; producer++) {
      queue.emplace(producer, i);
    }
  }
  {
    std::jthread consumer([&queue] { consume_all(queue, true, false); });
    std::vector<std::jthread> producers;
    producers.reserve(constProducerCount);
    for (int producer = 0; producer < constProducerCount; producer++) {
      producers.emplace_back([&queue, producer] {
        for (int i = constPrefillPerProducer; i < constIterPerProducer; i++) {
          while (!queue.produce([&](Message& message) { message = {producer, i}; })) {
            std::this_thread::yield();
          }
        }
      });
    }
  }
  ASSERT_TRUE(queue.empty());
}
