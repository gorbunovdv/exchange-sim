#pragma once

#include "base/logging/logging_backend.h"
#include "base/threading/cache_line.h"

#include <atomic>
#include <memory>

namespace exchange_sim {

template<typename ObjectType>
  requires std::is_trivially_destructible_v<ObjectType>
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
class MPSCQueue {
public:
  static_assert(sizeof(ObjectType) <= constCacheLineSize);

  explicit MPSCQueue(size_t size): capacity_(size), queue_(std::allocator<ObjectType>().allocate(size)) {}

  ~MPSCQueue() { std::allocator<ObjectType>().deallocate(queue_, capacity_); }

  bool produce(auto&& producer) {
    size_t current_head = head_.load(std::memory_order_acquire);
    for (;;) {
      if (next(current_head) == tail_.load(std::memory_order_acquire)) {
        if (size_t actual_head = head_.load(std::memory_order_acquire); current_head != actual_head) {
          current_head = actual_head;
          continue;
        }
        return false;
      }
      if (head_.compare_exchange_weak(
              current_head, next(current_head), std::memory_order_release, std::memory_order_acquire)) {
        break;
      }
    }
    std::forward<decltype(producer)>(producer)(queue_[current_head]);

    size_t published_head = current_head;
    while (!published_head_.compare_exchange_weak(
        published_head, next(published_head), std::memory_order_release, std::memory_order_acquire)) {
      published_head = current_head;
    }
    return true;
  }

  bool consume(auto&& consumer) {
    size_t current_tail = tail_.load(std::memory_order_relaxed);
    if (current_tail == published_head_.load(std::memory_order_acquire)) {
      return false;
    }
    std::forward<decltype(consumer)>(consumer)(queue_[current_tail]);
    tail_.store(next(current_tail), std::memory_order_release);
    return true;
  }

  bool empty() const { return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire); }

  bool try_push(ObjectType&& object) { return try_emplace(std::move(object)); }

  void push(ObjectType&& object) { CHECK(try_push(std::move(object)), "Push failed, queue is full"); }

  bool try_emplace(auto&&... args) {
    return produce([&](ObjectType& place) { std::construct_at(&place, std::forward<decltype(args)>(args)...); });
  }

  void emplace(auto&&... args) {
    CHECK(try_emplace(std::forward<decltype(args)>(args)...), "Push failed, queue is full");
  }

  bool try_pop(ObjectType& object) {
    return consume([&](ObjectType& place) { object = std::move(place); });
  }

private:
  constexpr size_t next(size_t index) const {
    ++index;
    return index == capacity_ ? 0 : index;
  }

  size_t capacity_;
  ObjectType* queue_;

  alignas(constCacheLineSeparationSize) std::atomic<size_t> head_ = 0;
  alignas(constCacheLineSeparationSize) std::atomic<size_t> published_head_ = 0;
  alignas(constCacheLineSeparationSize) std::atomic<size_t> tail_ = 0;
};

}  // namespace exchange_sim