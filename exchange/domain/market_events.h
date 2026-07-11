#pragma once

#include "base/processor.h"
#include "base/sink.h"
#include "base/time/rdtsc.h"

#include "domain/security.h"
#include "domain/types.h"

#include "exchange/domain/types.h"

#include <variant>

namespace exchange_sim::market_events {

struct BookOrderEntry {
  Ticks origin_time;
  ExchangeOrderID order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount quantity;

  bool operator==(const BookOrderEntry& other) const = default;
};

struct BookOrderChange {
  Ticks origin_time;
  ExchangeOrderID order_id;
  SecurityID security_id;
  Direction direction;
  Price price;
  Amount new_quantity;

  bool operator==(const BookOrderChange& other) const = default;
};

struct BookOrderDeletion {
  Ticks origin_time;
  ExchangeOrderID order_id;
  SecurityID security_id;

  bool operator==(const BookOrderDeletion& other) const = default;
};

struct Trade {
  Ticks origin_time;
  SecurityID security_id;
  Price price;
  Amount quantity;
  ExchangeTradeID trade_id;
  ExchangeOrderID aggressor_order_id;
  ExchangeOrderID recipient_order_id;

  bool operator==(const Trade& other) const = default;
};

using EventType = std::variant<BookOrderEntry, BookOrderChange, BookOrderDeletion, Trade>;

template<typename SinkType>
concept Sink = SinkOf<SinkType, EventType>;

template<typename ProcessorType>
concept Processor = ProcessorOf<ProcessorType, EventType>;

}  // namespace exchange_sim::market_events
