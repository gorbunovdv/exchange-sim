#pragma once

#include "domain/engine_snapshots.h"
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
using OrderExpired = engine_snapshot::OrderExpired;
using OrderCancelled = engine_snapshot::OrderCancelled;
using OrderRejected = engine_snapshot::OrderRejected;
using CancellationRejected = engine_snapshot::CancellationRejected;
using ModificationAccepted = engine_snapshot::ModificationAccepted;
using ModificationRejected = engine_snapshot::ModificationRejected;

using OrderRejectedReason = engine_snapshot::OrderRejectedReason;
using CancellationRejectedReason = engine_snapshot::CancellationRejectedReason;
using ModificationRejectedReason = engine_snapshot::ModificationRejectedReason;

using ReportType = std::variant<OrderAccepted,
    Trade,
    OrderBookEntry,
    OrderExpired,
    OrderCancelled,
    OrderRejected,
    CancellationRejected,
    ModificationAccepted,
    ModificationRejected>;

}  // namespace exchange_sim::report