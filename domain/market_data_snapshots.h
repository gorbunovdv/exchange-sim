#pragma once

#include "domain/types.h"
#include <variant>

namespace exchange_sim::market_data {

struct OrderEntrySnapshot {
  ExchangeOrderID order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount quantity;
};

struct OrderChangeSnapshot {
  ExchangeOrderID order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount new_quantity;
};

struct OrderCancellationSnapshot {
  ExchangeOrderID order_id;
  SecurityID security_id;
};

struct TradeSnapshot {
  SecurityID security_id;
  Price price;
  Amount quantity;
  ExchangeTradeID trade_id;
  ExchangeOrderID aggressor_order_id;
  ExchangeOrderID recipient_order_id;
};

using SnapshotType = std::variant<OrderEntrySnapshot, OrderChangeSnapshot, OrderCancellationSnapshot, TradeSnapshot>;

}  // namespace exchange_sim::market_data