#pragma once
#include "base/indexed_map.h"
#include "base/logging/logging_backend.h"

#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"
#include "exchange/domain/types.h"

namespace exchange_sim {

struct Order {
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;

  enum class State : std::uint8_t { SENT, ACCEPTED, ACTIVE, CANCEL_SENT, REPLACE_SENT };

  State state;
  bool filled;
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

  size_t pending_transactions() const { return pending_transactions_count_; }

  const auto& orders() const { return orders_; }

  const auto& active_orders() const { return active_orders_; }

private:
  using PairHash = decltype([](const std::pair<ClientID, ClientOrderID>& key) {
    return std::hash<uint64_t>()((std::hash<ClientID>()(key.first) << 32) ^ std::hash<ClientOrderID>()(key.second));
  });

  using OrderContainer = std::unordered_map<std::pair<ClientID, ClientOrderID>, Order, PairHash>;
  using ActiveOrderContainer = IndexedMap<std::pair<ClientID, ClientOrderID>, Order, PairHash>;

  OrderContainer::iterator find_order(ClientID client_id, ClientOrderID client_order_id);

  size_t pending_transactions_count_ = 0;
  OrderContainer orders_;
  ActiveOrderContainer active_orders_;
};

}  // namespace exchange_sim
