#pragma once

#include <list>

namespace exchange_sim {

template<typename ActiveOrder>
class StlL3BookLevel {
public:
  using iterator = std::list<ActiveOrder>::iterator;

  bool empty() const { return level_.empty(); }

  auto& front() { return level_.front(); }

  iterator push_back(const ActiveOrder& order) {
    level_.push_back(order);
    return std::prev(level_.end());
  }

  void erase_front() { level_.pop_front(); }

  void erase(iterator iterator) { level_.erase(iterator); }

private:
  std::list<ActiveOrder> level_;
};

template<typename ActiveOrder>
class StlL3BookContainer {
public:
  using BookLevel = StlL3BookLevel<ActiveOrder>;

  template<typename Comparator>
  using HalfBook = std::map<Price, BookLevel, Comparator>;

  using BuyBook = HalfBook<std::greater<Price>>;
  using SellBook = HalfBook<std::less<Price>>;

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
  void push_back(const ActiveOrder& order) {
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

  std::optional<ActiveOrder> extract_order(ClientID client_id, ClientOrderID order_id) {
    auto iterator = active_orders_.find(std::make_pair(client_id, order_id));
    if (iterator == active_orders_.end()) {
      return std::nullopt;
    }
    auto order_iterator = iterator->second;
    ActiveOrder current_order = *order_iterator;
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
    return std::hash<uint64_t>()((static_cast<uint64_t>(key.first) << 32) ^ std::hash<ClientOrderID>()(key.second));
  });

  template<Direction direction>
  void erase_from_level(Price price, StlL3BookLevel<ActiveOrder>::iterator iterator) {
    auto it = half<direction>().find(price);
    it->second.erase(iterator);
    if (it->second.empty()) {
      half<direction>().erase(it);
    }
  }

  BuyBook buy_;
  SellBook sell_;
  std::unordered_map<std::pair<ClientID, ClientOrderID>, typename BookLevel::iterator, PairHash> active_orders_;
};

}  // namespace exchange_sim
