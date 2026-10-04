#pragma once

#include "base/logging/logging_backend.h"

#include <boost/intrusive/slist.hpp>

#include <array>
#include <vector>

namespace exchange_sim {

template<size_t Size, size_t Alignment>
class PlaceArena : NonCopyable, NonMoveable {
public:
  explicit PlaceArena(size_t capacity = 0) { reserve(capacity); }

  ~PlaceArena() = default;

  void* allocate(size_t n) {
    CHECK(n <= Size, "Object size exceeds arena object size, object_size={} arena_object_size={}", n, Size);
    CHECK(!free_list_.empty(), "Arena is empty, object_size={} capacity={}", Size, capacity_);
    auto& element = free_list_.front();
    free_list_.pop_front();
    element.~Element();
    return static_cast<void*>(&element);
  }

  void deallocate(void* element, size_t) { free_list_.push_front(*new (element) Element); }

  void reserve(size_t capacity) {
    CHECK(storage_.empty(), "Cannot change capacity of non-empty arena");
    capacity_ = capacity;
    storage_.resize(capacity_);
    free_list_ = BoostList(storage_.begin(), storage_.end());
  }

private:
  using BoostBaseHook = boost::intrusive::slist_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>;

  struct alignas(Alignment) Element : BoostBaseHook {
    std::array<std::byte, Size - sizeof(BoostBaseHook)> data;
  };

  using BoostList = boost::intrusive::slist<Element, boost::intrusive::constant_time_size<false>>;

  static_assert(sizeof(Element) >= sizeof(BoostBaseHook));
  static_assert(Alignment >= alignof(BoostBaseHook));

  size_t capacity_;
  std::vector<Element> storage_;
  BoostList free_list_;
};

}  // namespace exchange_sim
