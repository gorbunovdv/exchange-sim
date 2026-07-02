#pragma once

#include "base/sink.h"
#include "domain/types.h"
#include "exchange/domain/types.h"
#include <variant>

namespace exchange_sim::market_data_snapshot {

struct BookOrderEntry {
  ExchangeOrderID order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount quantity;

  bool operator==(const BookOrderEntry& other) const = default;
};

struct BookOrderChange {
  ExchangeOrderID order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount new_quantity;

  bool operator==(const BookOrderChange& other) const = default;
};

struct BookOrderDeletion {
  ExchangeOrderID order_id;
  SecurityID security_id;

  bool operator==(const BookOrderDeletion& other) const = default;
};

struct Trade {
  SecurityID security_id;
  Price price;
  Amount quantity;
  ExchangeTradeID trade_id;
  ExchangeOrderID aggressor_order_id;
  ExchangeOrderID recipient_order_id;

  bool operator==(const Trade& other) const = default;
};

using SnapshotType = std::variant<BookOrderEntry, BookOrderChange, BookOrderDeletion, Trade>;

template<typename SinkType>
concept Sink = SinkOf<SinkType, SnapshotType>;

template<typename ProcessorType>
concept Processor = ProcessorOf<ProcessorType, SnapshotType>;

}  // namespace exchange_sim::market_data_snapshot