#pragma once

#include "base/sink.h"
#include "exchange/domain/engine_snapshots.h"
#include "exchange/domain/types.h"
#include "domain/types.h"
#include <variant>

namespace exchange_sim::report {

using OrderAccepted = engine_snapshot::OrderAccepted;

struct Trade {
  SecurityID security_id;
  Direction aggressor_direction;
  Price price;
  Amount quantity;
  ClientID client_id;
  ClientOrderID client_order_id;
  ExchangeTradeID trade_id;
  ExchangeOrderID aggressor_order_id;
  ExchangeOrderID recipient_order_id;
};

using OrderBookEntry = engine_snapshot::OrderBookEntry;
using OrderFilled = engine_snapshot::OrderFilled;
using OrderExpired = engine_snapshot::OrderExpired;
using OrderCancelled = engine_snapshot::OrderCancelled;
using OrderRejected = engine_snapshot::OrderRejected;
using CancellationRejected = engine_snapshot::CancellationRejected;
using ReplacementAccepted = engine_snapshot::ReplacementAccepted;
using ReplacementRejected = engine_snapshot::ReplacementRejected;

using OrderRejectedReason = engine_snapshot::OrderRejectedReason;
using CancellationRejectedReason = engine_snapshot::CancellationRejectedReason;
using ReplacementRejectedReason = engine_snapshot::ReplacementRejectedReason;

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