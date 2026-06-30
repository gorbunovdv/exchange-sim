#pragma once

#include "domain/exchange_snapshots.h"
#include "domain/market_data_snapshots.h"
#include "domain/reports.h"

#include <glaze/glaze.hpp>

namespace exchange_sim {

template<template<typename> class MDSinkType, template<typename> class ORSinkType>
class ExchangeDispatcher {
public:
  explicit ExchangeDispatcher(const glz::generic&,
      MDSinkType<market_data::SnapshotType>& market_data_sink,
      ORSinkType<report::ReportType>& report_generator_sink):
      market_data_sink_(market_data_sink), report_generator_sink_(report_generator_sink) {}

  void process(exchange_snapshot::SnapshotType snapshot) {
    std::visit([this](auto snapshot) { this->process_object(std::move(snapshot)); }, snapshot);
  }

private:
  void process_object(exchange_snapshot::OrderEntrySnapshot snapshot) {
    market_data_sink_.push(market_data::OrderEntrySnapshot{
        .order_id = snapshot.order_id,
        .security_id = snapshot.security_id,
        .time_in_force = snapshot.time_in_force,
        .order_type = snapshot.order_type,
        .direction = snapshot.direction,
        .price = snapshot.price,
        .quantity = snapshot.quantity,
    });
    report_generator_sink_.push(report::OrderEntryReport{
        .client_id = snapshot.client_id,
        .security_id = snapshot.security_id,
        .order_id = snapshot.order_id,
        .client_order_id = snapshot.client_order_id,
        .quantity = snapshot.quantity,
    });
  }

  void process_object(exchange_snapshot::OrderCancellationSnapshot snapshot) {
    market_data_sink_.push(market_data::OrderCancellationSnapshot{
        .order_id = snapshot.order_id,
        .security_id = snapshot.security_id,
    });
    report_generator_sink_.push(report::OrderCancellationReport{
        .client_id = snapshot.client_id,
        .security_id = snapshot.security_id,
        .order_id = snapshot.order_id,
        .client_order_id = snapshot.client_order_id,
        .client_cancellation_id = snapshot.client_cancellation_id,
    });
  }

  void process_object(exchange_snapshot::TradeSnapshot snapshot) {
    market_data_sink_.push(market_data::TradeSnapshot{
        .security_id = snapshot.security_id,
        .price = snapshot.price,
        .quantity = snapshot.quantity,
        .trade_id = snapshot.trade_id,
        .aggressor_order_id = snapshot.aggressor_order_id,
        .recipient_order_id = snapshot.recipient_order_id,
    });
    report_generator_sink_.push(report::OrderTradeReport{
        .order_id = snapshot.aggressor_order_id,
        .security_id = snapshot.security_id,
        .client_id = snapshot.aggressor_client_id,
        .client_order_id = snapshot.aggressor_client_order_id,
        .price = snapshot.price,
        .quantity = snapshot.quantity,
        .trade_id = snapshot.trade_id,
        .aggressor_order_id = snapshot.aggressor_order_id,
        .recipient_order_id = snapshot.recipient_order_id,
    });
    report_generator_sink_.push(report::OrderTradeReport{
        .order_id = snapshot.recipient_order_id,
        .security_id = snapshot.security_id,
        .client_id = snapshot.recipient_client_id,
        .client_order_id = snapshot.recipient_client_order_id,
        .price = snapshot.price,
        .quantity = snapshot.quantity,
        .trade_id = snapshot.trade_id,
        .aggressor_order_id = snapshot.aggressor_order_id,
        .recipient_order_id = snapshot.recipient_order_id,
    });
  }

  void process_object(exchange_snapshot::OrderRejectedSnapshot snapshot) {
    report_generator_sink_.push(report::OrderRejectedReport{
        .security_id = snapshot.security_id,
        .client_id = snapshot.client_id,
        .client_order_id = snapshot.client_order_id,
        .reason = convert_rejection_reason(snapshot.reason),
    });
  }

  void process_object(exchange_snapshot::CancellationRejectedSnapshot snapshot) {
    report_generator_sink_.push(report::CancellationRejectedReport{
        .security_id = snapshot.security_id,
        .client_id = snapshot.client_id,
        .client_order_id = snapshot.client_order_id,
        .client_cancellation_id = snapshot.client_cancellation_id,
        .reason = convert_cancellation_rejected_reason(snapshot.reason),
    });
  }

  report::OrderRejectedReason convert_rejection_reason(exchange_snapshot::OrderRejectedReason reason) {
    switch (reason) {
      case exchange_snapshot::OrderRejectedReason::INSUFFICIENT_FUNDS:
        return report::OrderRejectedReason::INSUFFICIENT_FUNDS;
      case exchange_snapshot::OrderRejectedReason::PRICE_OUT_OF_RANGE:
        return report::OrderRejectedReason::PRICE_OUT_OF_RANGE;
      default:
        CRITICAL("Unknown rejection reason: {}", static_cast<int>(reason));
    }
  }

  report::CancellationRejectedReason convert_cancellation_rejected_reason(
      exchange_snapshot::CancellationRejectedReason reason) {
    switch (reason) {
      case exchange_snapshot::CancellationRejectedReason::ORDER_NOT_FOUND:
        return report::CancellationRejectedReason::ORDER_NOT_FOUND;
      case exchange_snapshot::CancellationRejectedReason::PRICE_OUT_OF_RANGE:
        return report::CancellationRejectedReason::PRICE_OUT_OF_RANGE;
      case exchange_snapshot::CancellationRejectedReason::INSUFFICIENT_FUNDS:
        return report::CancellationRejectedReason::INSUFFICIENT_FUNDS;
      default:
        CRITICAL("Unknown cancellation rejected reason: {}", static_cast<int>(reason));
    }
  }

  MDSinkType<market_data::SnapshotType>& market_data_sink_;
  ORSinkType<report::ReportType>& report_generator_sink_;
};

}  // namespace exchange_sim