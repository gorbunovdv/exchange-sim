#pragma once

#include "application/types.h"
#include <variant>

namespace exchange_sim {

struct OrderEntrySnapshot {
  ExchangeOrderID order_id;
  SecurityID symbol_id;
  OrderTimeInForce time_in_force;
  OrderType order_type;
  Direction direction;
  Price price;
  Quantity quantity;
};

struct OrderCancellationSnapshot {
  ExchangeOrderID order_id;
  SecurityID symbol_id;
};

struct TradeSnapshot {
  SecurityID symbol_id;
  Price price;
  Quantity quantity;
  ExchangeTradeID trade_id;
  ExchangeOrderID aggressor_order_id;
  ExchangeOrderID recipient_order_id;
};

using MarketDataSnapshotType = std::variant<OrderEntrySnapshot, OrderCancellationSnapshot, TradeSnapshot>;

}  // namespace exchange_sim