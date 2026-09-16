#pragma once

#include "base/sink.h"

#include "domain/types.h"

#include "exchange/domain/engine_events.h"
#include "exchange/domain/types.h"

#include <variant>

namespace exchange_sim::report {

using OrderAccepted = engine_events::OrderAccepted;

struct Trade {
  Ticks origin_time;
  SecurityID security_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  ExchangeTradeID trade_id;
  ExchangeOrderID aggressor_order_id;
  ExchangeOrderID recipient_order_id;
  Price price;
  Amount quantity;
  Direction aggressor_direction;

  bool operator==(const Trade& other) const = default;
};

using OrderBookEntry = engine_events::OrderBookEntry;
using OrderFilled = engine_events::OrderFilled;
using OrderExpired = engine_events::OrderExpired;
using OrderCancelled = engine_events::OrderCancelled;
using OrderRejected = engine_events::OrderRejected;
using CancellationRejected = engine_events::CancellationRejected;
using ReplacementAccepted = engine_events::ReplacementAccepted;
using ReplacementRejected = engine_events::ReplacementRejected;

using OrderRejectedReason = engine_events::OrderRejectedReason;
using CancellationRejectedReason = engine_events::CancellationRejectedReason;
using ReplacementRejectedReason = engine_events::ReplacementRejectedReason;

using ReportType = std::variant<OrderAccepted,
    Trade,
    OrderBookEntry,
    OrderFilled,
    OrderExpired,
    OrderCancelled,
    OrderRejected,
    CancellationRejected,
    ReplacementAccepted,
    ReplacementRejected>;

template<typename SinkType>
concept Sink = SinkOf<SinkType, ReportType>;

template<typename ProcessorType>
concept Processor = ProcessorOf<ProcessorType, ReportType>;

}  // namespace exchange_sim::report
