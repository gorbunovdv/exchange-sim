#pragma once

#include "domain/types.h"

#include "exchange/domain/types.h"

#include <list>
#include <map>

namespace exchange_sim {

template<typename ActiveOrder>
class StlL3BookLevel {
public:
  using OrderType = ActiveOrder;
  using iterator = std::list<OrderType>::iterator;

  bool empty() const { return level_.empty(); }

  auto& front() { return level_.front(); }

  iterator push_back(const OrderType& order) {
    level_.push_back(order);
    return std::prev(level_.end());
  }

  void erase_front() { level_.pop_front(); }

  void erase(iterator iterator) { level_.erase(iterator); }

private:
  std::list<OrderType> level_;
};

template<typename ActiveOrder>
class StlL3BookContainer {
public:
  using OrderType = ActiveOrder;
  using BookLevelType = StlL3BookLevel<ActiveOrder>;

  template<typename Comparator>
  using HalfBookType = std::map<Price, BookLevelType, Comparator>;

  using BuyBook = HalfBookType<std::greater<Price>>;
  using SellBook = HalfBookType<std::less<Price>>;

  bool contains(ClientID client_id, ClientOrderID order_id) const {
    return active_orders_.contains(std::make_pair(client_id, order_id));
  }

  template<Direction direction>
  constexpr auto& half() {
    if constexpr (direction == Direction::BUY) {
      return buy_;
    } else {
      return sell_;
    }
  }

  template<Direction direction>
  constexpr auto& opposite_half() {
    if constexpr (direction == Direction::BUY) {
      return sell_;
    } else {
      return buy_;
    }
  }

  template<Direction direction>
  void push_back(const OrderType& order) {
    active_orders_.emplace(
        std::make_pair(order.client_id, order.client_order_id), half<direction>()[order.price].push_back(order));
  }

  template<Direction direction>
  void erase_front() {
    auto& current_level = half<direction>().begin()->second;
    auto& current_order = current_level.front();
    active_orders_.erase(std::make_pair(current_order.client_id, current_order.client_order_id));
    current_level.erase_front();
  }

  std::optional<OrderType> extract_order(ClientID client_id, ClientOrderID order_id) {
    auto iterator = active_orders_.find(std::make_pair(client_id, order_id));
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

private:
  using PairHash = decltype([](const std::pair<ClientID, ClientOrderID>& key) {
    return std::hash<uint64_t>()((std::hash<ClientID>()(key.first) << 32) ^ std::hash<ClientOrderID>()(key.second));
  });

  template<Direction direction>
  void erase_from_level(Price price, StlL3BookLevel<OrderType>::iterator iterator) {
    auto it = half<direction>().find(price);
    it->second.erase(iterator);
    if (it->second.empty()) {
      half<direction>().erase(it);
    }
  }

  BuyBook buy_;
  SellBook sell_;
  std::unordered_map<std::pair<ClientID, ClientOrderID>, typename BookLevelType::iterator, PairHash> active_orders_;
};

}  // namespace exchange_sim
