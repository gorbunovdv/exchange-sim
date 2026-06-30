#pragma once

#include "domain/types.h"
#include <variant>

namespace exchange_sim::exchange_snapshot {

struct OrderEntrySnapshot {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;
  OrderTimeInForce time_in_force;
  OrderType order_type;
  Direction direction;
  Price price;
  Amount quantity;
};

struct OrderCancellationSnapshot {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_cancellation_id;
  SecurityID security_id;
};

struct TradeSnapshot {
  SecurityID security_id;
  Price price;
  Amount quantity;
  ExchangeTradeID trade_id;
  ExchangeOrderID aggressor_order_id;
  ClientID aggressor_client_id;
  ClientOrderID aggressor_client_order_id;
  ExchangeOrderID recipient_order_id;
  ClientID recipient_client_id;
  ClientOrderID recipient_client_order_id;
};

enum class OrderRejectedReason : std::uint8_t { INSUFFICIENT_FUNDS, PRICE_OUT_OF_RANGE };

struct OrderRejectedSnapshot {
  SecurityID security_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  OrderRejectedReason reason;
};

enum class CancellationRejectedReason : std::uint8_t { ORDER_NOT_FOUND, INSUFFICIENT_FUNDS, PRICE_OUT_OF_RANGE };

struct CancellationRejectedSnapshot {
  ExchangeOrderID order_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_cancellation_id;
  SecurityID security_id;
  CancellationRejectedReason reason;
};

using SnapshotType = std::variant<OrderEntrySnapshot,
    OrderCancellationSnapshot,
    TradeSnapshot,
    OrderRejectedSnapshot,
    CancellationRejectedSnapshot>;

}  // namespace exchange_sim::exchange_snapshot