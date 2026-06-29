#pragma once

#include "application/types.h"

namespace exchange_sim::report {

enum class ReportOutcome : std::uint8_t { SUCCESS, FAILURE };

constexpr std::string_view format_as(ReportOutcome outcome) {
  return magic_enum::enum_name(outcome);
}

struct OrderEntryReport {
  ClientID client_id;
  SecurityID security_id;
  ExchangeOrderID order_id;
  ClientOrderID client_order_id;
  Amount quantity;
};

struct OrderCancellationReport {
  ClientID client_id;
  SecurityID security_id;
  ExchangeOrderID order_id;
  ClientOrderID client_order_id;
  ClientOrderID client_cancellation_id;
};

struct OrderTradeReport {
  ExchangeOrderID order_id;
  SecurityID security_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  Price price;
  Amount quantity;
  ExchangeTradeID trade_id;
  ExchangeOrderID aggressor_order_id;
  ExchangeOrderID recipient_order_id;
};

enum class OrderRejectedReason : std::uint8_t { INSUFFICIENT_FUNDS, PRICE_OUT_OF_RANGE };

struct OrderRejectedReport {
  SecurityID security_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  OrderRejectedReason reason;
};

enum class CancellationRejectedReason : std::uint8_t { ORDER_NOT_FOUND, INSUFFICIENT_FUNDS, PRICE_OUT_OF_RANGE };

struct CancellationRejectedReport {
  SecurityID security_id;
  ClientID client_id;
  ClientOrderID client_order_id;
  ClientOrderID client_cancellation_id;
  CancellationRejectedReason reason;
};

using ReportType = std::variant<OrderEntryReport,
    OrderCancellationReport,
    OrderTradeReport,
    OrderRejectedReport,
    CancellationRejectedReport>;

}  // namespace exchange_sim::report