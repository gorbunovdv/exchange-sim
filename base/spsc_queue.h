#pragma once

#include "base/logging/logging_backend.h"
#include "base/threading/cache_line.h"

#include <atomic>
#include <span>

namespace exchange_sim {

template<typename ObjectType>
class SPSCQueue {
public:
  explicit SPSCQueue(auto&&... args): queue_(std::forward<decltype(args)>(args)...) {}

  template<typename Producer>
  bool produce(Producer&& producer) {
    // todo(gorbunovdv): Use correct memory order
    size_t current_tail = tail_.load(), current_head = head_.load();
    size_t next_head = (current_head + 1) % queue_.size();
    if (next_head == current_tail) {
      return false;
    }
    std::forward<Producer>(producer)(queue_[current_head]);
    head_.store(next_head);
    return true;
  }

  template<typename Consumer>
  void consume_span(Consumer&& consumer) {
    size_t current_head = head_.load(), current_tail = tail_.load();
    size_t last_head = current_tail <= current_head ? current_head : queue_.size();
    std::forward<Consumer>(consumer)(std::span{queue_.data() + current_tail, queue_.data() + last_head});
    tail_.store(last_head % queue_.size());
  }

  template<typename Consumer>
  bool consume(Consumer&& consumer) {
    size_t current_head = head_.load(), current_tail = tail_.load();
    if (current_head == current_tail) {
      return false;
    }
    std::forward<Consumer>(consumer)(queue_[current_tail]);
    tail_.store((current_tail + 1) % queue_.size());
    return true;
  }

  bool try_push(ObjectType&& object) {
    return produce([&](ObjectType& place) { place = std::move(object); });
  }

  void push(ObjectType&& object) { CHECK(try_push(std::move(object)), "Push failed, queue is full"); }

  bool try_pop(ObjectType& object) {
    return consume([&](ObjectType& place) { object = std::move(place); });
  }

  bool empty() const { return head_.load() == tail_.load(); }

  size_t size() const {
    size_t current_head = head_.load(), current_tail = tail_.load();
    if (current_head <= current_tail) {
      return current_tail - current_head;
    }
    return queue_.size() - (current_head - current_tail);
  }

private:
  struct alignas(ObjectType) AlignedType {
    std::array<std::byte, sizeof(ObjectType)> placeholder;
  };

  static_assert(sizeof(AlignedType) == sizeof(ObjectType), "AlignedType should have the same size as value_type");

  std::vector<ObjectType> queue_;

  alignas(constCacheLineSize) std::atomic<size_t> head_ = 0;
  alignas(constCacheLineSize) std::atomic<size_t> tail_ = 0;
};

}  // namespace exchange_sim
