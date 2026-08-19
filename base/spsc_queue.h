#pragma once

#include "base/logging/logging_backend.h"
#include "base/threading/cache_line.h"

#include <atomic>
#include <span>

namespace exchange_sim {

template<typename ObjectType>
  requires std::is_trivially_destructible_v<ObjectType>
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
class SPSCQueue {
public:
  explicit SPSCQueue(size_t size): capacity_(size), queue_(std::allocator<ObjectType>().allocate(size)) {}

  ~SPSCQueue() { std::allocator<ObjectType>().deallocate(queue_, capacity_); }

  template<typename Producer>
  bool produce(Producer&& producer) {
    size_t current_head = head_.load(std::memory_order_relaxed);
    size_t next_head = (current_head + 1) % capacity_;
    if (next_head == cached_tail_) {
      cached_tail_ = tail_.load(std::memory_order_acquire);
      if (next_head == cached_tail_) {
        return false;
      }
    }
    std::forward<Producer>(producer)(queue_[current_head]);
    head_.store(next_head, std::memory_order_release);
    return true;
  }

  template<typename Consumer>
  auto consume_span(Consumer&& consumer) {
    size_t current_tail = tail_.load(std::memory_order_relaxed);
    if (current_tail == cached_head_) {
      cached_head_ = head_.load(std::memory_order_acquire);
    }
    size_t last_head = current_tail <= cached_head_ ? cached_head_ : capacity_;
    auto result = std::forward<Consumer>(consumer)(std::span{queue_ + current_tail, queue_ + last_head});
    if (current_tail != last_head) {
      tail_.store(last_head % capacity_, std::memory_order_release);
    }
    return result;
  }

  template<typename Consumer>
  bool consume(Consumer&& consumer) {
    size_t current_tail = tail_.load(std::memory_order_relaxed);
    if (cached_head_ == current_tail) {
      cached_head_ = head_.load(std::memory_order_acquire);
      if (cached_head_ == current_tail) {
        return false;
      }
    }
    std::forward<Consumer>(consumer)(queue_[current_tail]);
    tail_.store((current_tail + 1) % capacity_, std::memory_order_release);
    return true;
  }

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

  bool empty() const { return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire); }

  size_t capacity() const { return capacity_ - 1; }

  size_t size() const {
    size_t current_head = head_.load(std::memory_order_acquire);
    size_t current_tail = tail_.load(std::memory_order_acquire);
    if (current_tail <= current_head) {
      return current_head - current_tail;
    }
    return capacity_ - (current_tail - current_head);
  }

private:
  size_t capacity_;
  ObjectType* queue_;

  alignas(constCacheLineSize) std::atomic<size_t> head_ = 0;
  alignas(constCacheLineSize) std::atomic<size_t> tail_ = 0;

  alignas(constCacheLineSize) size_t cached_head_ = 0;
  alignas(constCacheLineSize) size_t cached_tail_ = 0;
};

}  // namespace exchange_sim
