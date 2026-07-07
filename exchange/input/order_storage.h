#pragma once
#include "base/logging/logging_backend.h"

#include "domain/security_storage.h"

#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"
#include "exchange/domain/types.h"

#include <list>
#include <ranges>

namespace exchange_sim {

struct Order {
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;

  enum class State : std::uint8_t { SENT, ACCEPTED, ACTIVE, CANCEL_SENT, REPLACE_SENT };

  State state;
};

constexpr std::string_view format_as(Order::State state) {
  return magic_enum::enum_name(state);
}

class OrderStorage {
public:
  OrderStorage() = default;

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

  bool empty() const { return size() == 0; }

  size_t size() const {
    return std::ranges::count_if(orders_, [](const auto& order) { return order.state == Order::State::ACTIVE; });
  }

  size_t pending_transactions() const {
    return std::ranges::count_if(orders_, [](const auto& order) {
      return order.state == Order::State::SENT || order.state == Order::State::ACCEPTED ||
          order.state == Order::State::CANCEL_SENT || order.state == Order::State::REPLACE_SENT;
    });
  }

  const Order& operator[](size_t index) const;

  const auto& orders() const { return orders_; }

private:
  std::list<Order>::iterator find_order_or_end(ClientID client_id, ClientOrderID client_order_id);

  std::list<Order>::iterator find_order(ClientID client_id, ClientOrderID client_order_id);

  std::list<Order> orders_;

  using PairHash = decltype([](const std::pair<ClientID, ClientOrderID>& key) {
    return std::hash<uint64_t>()((static_cast<uint64_t>(key.first) << 32) ^ std::hash<ClientOrderID>()(key.second));
  });

  std::unordered_map<std::pair<ClientID, ClientOrderID>, decltype(orders_)::iterator, PairHash> order_map_;
};

}  // namespace exchange_sim