#pragma once
#include "base/indexed_map.h"
#include "base/logging/logging_backend.h"

#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"
#include "exchange/domain/types.h"

#include <magic_enum/magic_enum_containers.hpp>

namespace exchange_sim {

class OrderStorage {
public:
  struct Order {
    using UniqueID = std::pair<ClientID, ClientOrderID>;

    ClientID client_id;
    ClientOrderID client_order_id;
    SecurityID security_id;
    Direction direction;

    enum class State : std::uint8_t { SENT, ACCEPTED, ACTIVE, CANCEL_SENT, REPLACE_SENT };

    State state;
    bool filled;

    UniqueID id() const { return {client_id, client_order_id}; }
  };

  void handle(const request::OrderAddRequest& request);
  void handle(const request::OrderCancelRequest& request);
  void handle(const request::OrderReplaceRequest& request);
  void handle(const report::OrderAccepted& report);
  void handle(const report::OrderRejected& report);
  void handle(const report::OrderBookEntry& report);
  void handle(const report::OrderExpired& report);
  void handle(const report::OrderFilled& report);
  void handle(const report::OrderCancelled& report);
  void handle(const report::CancellationRejected& report);
  void handle(const report::ReplacementAccepted& report);
  void handle(const report::ReplacementRejected& report);

  size_t pending_transactions() const { return pending_transactions_count_; }

  const auto& orders() const { return orders_; }

  const auto& active_orders() const { return active_orders_; }

  void reserve(size_t size) {
    orders_.reserve(size);
    active_orders_.reserve(size);
  }

private:
  using OrderContainer = std::unordered_map<std::pair<ClientID, ClientOrderID>, Order>;

  class ActiveOrderContainerWrapper {
  public:
    using ActiveOrderContainer = IndexedMap<Order::UniqueID, Order>;

    size_t size() const { return orders_[Direction::BUY].size() + orders_[Direction::SELL].size(); }

    bool empty() const { return orders_[Direction::BUY].empty() && orders_[Direction::SELL].empty(); }

    auto&& orders_by_direction(this auto&& self, Direction direction) {
      return std::forward_like<decltype(self)>(self.orders_)[direction];
    }

    void erase(Direction direction, const Order::UniqueID& key) { orders_[direction].erase(key); }

    void emplace(Direction direction, auto&&... args) {
      orders_[direction].emplace(std::forward<decltype(args)>(args)...);
    }

    auto&& operator[](this auto&& self, size_t index) {
      size_t buy_size = self.orders_[Direction::BUY].size();
      return index < buy_size ? std::forward_like<decltype(self)>(self.orders_)[Direction::BUY][index] :
                                std::forward_like<decltype(self)>(self.orders_)[Direction::SELL][index - buy_size];
    }

    void reserve(size_t size) {
      for (auto& container : orders_) {
        container.reserve(size);
      }
    }

  private:
    magic_enum::containers::array<Direction, ActiveOrderContainer> orders_;
  };

  OrderContainer::iterator find_order(ClientID client_id, ClientOrderID client_order_id);

  size_t pending_transactions_count_ = 0;
  OrderContainer orders_;
  ActiveOrderContainerWrapper active_orders_;
};

constexpr std::string_view format_as(OrderStorage::Order::State state) {
  return magic_enum::enum_name(state);
}

}  // namespace exchange_sim
