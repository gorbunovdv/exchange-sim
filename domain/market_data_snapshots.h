#pragma once

#include "domain/types.h"
#include <variant>

namespace exchange_sim::market_data_snapshot {

struct BookOrderEntry {
  ExchangeOrderID order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount quantity;
};

struct BookOrderChange {
  ExchangeOrderID order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount new_quantity;
};

struct BookOrderDeletion {
  ExchangeOrderID order_id;
  SecurityID security_id;
};

struct Trade {
  SecurityID security_id;
  Price price;
  Amount quantity;
  ExchangeTradeID trade_id;
  ExchangeOrderID aggressor_order_id;
  ExchangeOrderID recipient_order_id;
};

using SnapshotType = std::variant<BookOrderEntry, BookOrderChange, BookOrderDeletion, Trade>;

}  // namespace exchange_sim::market_data_snapshot