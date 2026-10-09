#pragma once

#include "base/spsc_queue.h"

namespace exchange_sim {

template<template<typename> typename QueueType, typename ObjectType>
class AlignedQueue {
public:
  explicit AlignedQueue(size_t capacity): queue_(capacity) {}

  template<typename Producer>
  bool produce(Producer&& producer) {
    return queue_.produce([&producer](AlignedObjectType& place) { std::forward<Producer>(producer)(place.object); });
  }

  template<typename Consumer>
  bool consume(Consumer&& consumer) {
    return queue_.consume([&consumer](AlignedObjectType& place) { std::forward<Consumer>(consumer)(place.object); });
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

  bool empty() const { return queue_.empty(); }

  size_t capacity() const { return queue_.capacity(); }

private:
  struct alignas(constCacheLineSize) AlignedObjectType {
    ObjectType object;
  };

  QueueType<AlignedObjectType> queue_;
};

}  // namespace exchange_sim