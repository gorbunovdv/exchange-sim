#pragma once

#include "base/allocators/custom_allocator.h"
#include "base/allocators/place_arena.h"
#include "base/threading/cache_line.h"

#include "domain/types.h"

#include "exchange/book_containers/book_container_concept.h"

#include <list>
#include <map>

namespace exchange_sim {

template<typename ActiveOrder>
class StlL3BookLevel {
public:
  using OrderType = ActiveOrder;
  using NodeType =
#if defined(_LIBCPP_VERSION)
      std::__list_node<OrderType, void*>;
#else
      std::_List_node<OrderType>;
#endif
  static constexpr size_t NodeSize = sizeof(NodeType);

  static_assert(NodeSize <= constCacheLineSize);
  
  using iterator = std::list<OrderType>::iterator;
  using Arena = PlaceArena<NodeSize, constCacheLineSize>;

  explicit StlL3BookLevel(Arena& arena): level_(CustomAllocator<ActiveOrder, Arena>(&arena)) {}

  bool empty() const { return level_.empty(); }

  auto& front() { return level_.front(); }

  iterator push_back(const OrderType& order) {
    level_.push_back(order);
    return std::prev(level_.end());
  }

  void erase_front() { level_.pop_front(); }

  void erase(iterator iterator) { level_.erase(iterator); }

  auto begin(this auto&& self) { return std::forward_like<decltype(self)>(self.level_).begin(); }
  auto end(this auto&& self) { return std::forward_like<decltype(self)>(self.level_).end(); }

private:
  std::list<OrderType, CustomAllocator<ActiveOrder, Arena>> level_;
};

template<typename ActiveOrder, Direction direction>
class StlL3HalfBook {
public:
  using LevelType = StlL3BookLevel<ActiveOrder>;
  using LevelArena = LevelType::Arena;

  using MapNodeType =
#if defined(_LIBCPP_VERSION)
      std::__tree_node<std::__value_type<Price, LevelType>, void*>;
#else
      std::_Rb_tree_node<std::pair<const Price, LevelType>>;
#endif
  using MapArena = PlaceArena<sizeof(MapNodeType), constCacheLineSize>;

  explicit StlL3HalfBook(MapArena& map_arena, LevelArena& level_arena):
      level_arena_(level_arena), half_book_(CustomAllocator<std::pair<const Price, LevelType>, MapArena>(&map_arena)) {}

  using Map = std::map<Price,
      LevelType,
      std::conditional_t<direction == Direction::BUY, std::greater<Price>, std::less<Price>>,
      CustomAllocator<std::pair<const Price, LevelType>, MapArena>>;

  using iterator = Map::iterator;
  using const_iterator = Map::const_iterator;

  auto key_comp() const { return half_book_.key_comp(); }
  bool empty() const { return half_book_.empty(); }

  auto begin(this auto&& self) { return self.half_book_.begin(); }
  auto end(this auto&& self) { return self.half_book_.end(); }
  auto find(this auto&& self, Price price) { return self.half_book_.find(price); }

  auto& operator[](Price price) { return half_book_.try_emplace(price, level_arena_).first->second; }

  iterator erase(const_iterator it) { return half_book_.erase(it); }

  Price best_price() const { return half_book_.empty() ? min_price() : half_book_.begin()->first; }

  void reserve(size_t) {}

private:
  static consteval Price min_price() {
    return direction == Direction::BUY ? std::numeric_limits<Price>::lowest() : std::numeric_limits<Price>::max();
  }

  LevelArena& level_arena_;
  Map half_book_;
};

template<BookRecord ActiveOrder>
class StlL3BookContainer {
public:
  using OrderType = ActiveOrder;
  using BookLevelType = StlL3BookLevel<ActiveOrder>;
  using UniqueID = decltype(std::declval<ActiveOrder>().id());

  explicit StlL3BookContainer(): buy_(map_arena_, level_arena_), sell_(map_arena_, level_arena_) {}

  bool contains(UniqueID id) const { return active_orders_.contains(id); }

  OrderType* find(UniqueID id) {
    if (auto it = active_orders_.find(id); it != active_orders_.end()) {
      return &(*it->second);
    }
    return nullptr;
  }

  template<Direction direction>
  constexpr auto& half(this auto&& self) {
    if constexpr (direction == Direction::BUY) {
      return std::forward_like<decltype(self)>(self.buy_);
    } else {
      return std::forward_like<decltype(self)>(self.sell_);
    }
  }

  template<Direction direction>
  constexpr auto& opposite_half(this auto&& self) {
    if constexpr (direction == Direction::BUY) {
      return std::forward_like<decltype(self)>(self.sell_);
    } else {
      return std::forward_like<decltype(self)>(self.buy_);
    }
  }

  template<Direction direction>
  void push_back(const OrderType& order) {
    auto it = half<direction>()[order.price].push_back(order);
    active_orders_.emplace(order.id(), it);
  }

  template<Direction direction>
  void erase_front() {
    auto& current_level = half<direction>().begin()->second;
    auto& current_order = current_level.front();
    active_orders_.erase(current_order.id());
    current_level.erase_front();
  }

  std::optional<OrderType> extract_order(UniqueID id) {
    auto iterator = active_orders_.find(id);
    if (iterator == active_orders_.end()) {
      return std::nullopt;
    }
    auto order_iterator = iterator->second;
    OrderType current_order = *order_iterator;
    active_orders_.erase(iterator);
    if (current_order.direction == Direction::BUY) {
      erase_from_level<Direction::BUY>(current_order.price, order_iterator);
    } else {
      erase_from_level<Direction::SELL>(current_order.price, order_iterator);
    }
    return current_order;
  }

  void reserve(size_t size) {
    active_orders_.reserve(size);
    map_arena_.reserve(size);
    level_arena_.reserve(size);
  }

private:
  template<Direction direction>
  void erase_from_level(Price price, StlL3BookLevel<OrderType>::iterator iterator) {
    auto it = half<direction>().find(price);
    it->second.erase(iterator);
    if (it->second.empty()) {
      half<direction>().erase(it);
    }
  }

  template<Direction direction>
  using HalfBookType = StlL3HalfBook<ActiveOrder, direction>;

  using LevelArena = typename HalfBookType<Direction::BUY>::LevelArena;
  using MapArena = typename HalfBookType<Direction::BUY>::MapArena;

  static_assert(std::is_same_v<LevelArena, typename HalfBookType<Direction::BUY>::LevelArena>);
  static_assert(std::is_same_v<LevelArena, typename HalfBookType<Direction::SELL>::LevelArena>);
  static_assert(std::is_same_v<MapArena, typename HalfBookType<Direction::BUY>::MapArena>);
  static_assert(std::is_same_v<MapArena, typename HalfBookType<Direction::SELL>::MapArena>);

  MapArena map_arena_;
  LevelArena level_arena_;
  StlL3HalfBook<ActiveOrder, Direction::BUY> buy_;
  StlL3HalfBook<ActiveOrder, Direction::SELL> sell_;
  std::unordered_map<UniqueID, typename BookLevelType::iterator> active_orders_;
};

}  // namespace exchange_sim
