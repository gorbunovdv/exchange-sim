#pragma once

namespace exchange_sim {

enum class ReportOutcome : std::uint8_t { SUCCESS, FAILURE };

constexpr std::string_view format_as(ReportOutcome outcome) {
  return magic_enum::enum_name(outcome);
}

struct OrderEntryReport {
  ExchangeOrderID order_id;
  ClientOrderID client_order_id;
  ReportOutcome outcome;
};

struct OrderCancellationReport {
  ExchangeOrderID order_id;
  ClientOrderID client_order_id;
};

struct OrderTradeReport {
  ExchangeOrderID order_id;
  ClientID client_order_id;
  Price price;
  Quantity quantity;
  ExchangeTradeID trade_id;
  ExchangeOrderID aggressor_order_id;
  ExchangeOrderID recipient_order_id;
};

using OrderReportType = std::variant<OrderEntryReport, OrderCancellationReport, OrderTradeReport>;

}  // namespace exchange_sim