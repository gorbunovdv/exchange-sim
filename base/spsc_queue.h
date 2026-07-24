#pragma once

#include "base/logging/logging_backend.h"
#include "base/threading/cache_line.h"

#include <atomic>
#include <span>

namespace exchange_sim {

template<typename ObjectType>
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
class SPSCQueue {
public:
  explicit SPSCQueue(size_t size): size_(size), queue_(std::allocator<ObjectType>().allocate(size)) {}

  ~SPSCQueue() { std::allocator<ObjectType>().deallocate(queue_, size_); }

  template<typename Producer>
  bool produce(Producer&& producer) {
    // todo(gorbunovdv): Use correct memory order
    size_t current_tail = tail_.load(), current_head = head_.load();
    size_t next_head = (current_head + 1) % size_;
    if (next_head == current_tail) {
      return false;
    }
    std::forward<Producer>(producer)(queue_[current_head]);
    head_.store(next_head);
    return true;
  }

  template<typename Consumer>
  auto consume_span(Consumer&& consumer) {
    size_t current_head = head_.load(), current_tail = tail_.load();
    size_t last_head = current_tail <= current_head ? current_head : size_;
    auto result = std::forward<Consumer>(consumer)(std::span{queue_ + current_tail, queue_ + last_head});
    if (current_tail != last_head) {
      tail_.store(last_head % size_);
    }
    return result;
  }

  template<typename Consumer>
  bool consume(Consumer&& consumer) {
    size_t current_head = head_.load(), current_tail = tail_.load();
    if (current_head == current_tail) {
      return false;
    }
    std::forward<Consumer>(consumer)(queue_[current_tail]);
    tail_.store((current_tail + 1) % size_);
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

  bool empty() const { return head_.load() == tail_.load(); }

  size_t size() const {
    size_t current_head = head_.load(), current_tail = tail_.load();
    if (current_tail <= current_head) {
      return current_head - current_tail;
    }
    return size_ - (current_tail - current_head);
  }

private:
  size_t size_;
  ObjectType* queue_;

  alignas(constCacheLineSize) std::atomic<size_t> head_ = 0;
  alignas(constCacheLineSize) std::atomic<size_t> tail_ = 0;
};

}  // namespace exchange_sim
