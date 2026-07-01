#pragma once

#include "domain/engine_snapshots.h"
#include "domain/market_data_snapshots.h"
#include "domain/reports.h"

#include <glaze/glaze.hpp>

namespace exchange_sim {

template<template<typename> class MDSinkType, template<typename> class ORSinkType>
class ExchangeDispatcher {
public:
  explicit ExchangeDispatcher(const glz::generic&,
      MDSinkType<market_data_snapshot::SnapshotType>& market_data_sink,
      ORSinkType<report::ReportType>& report_generator_sink):
      market_data_sink_(market_data_sink), report_generator_sink_(report_generator_sink) {}

  void process(engine_snapshot::SnapshotType snapshot) {
    std::visit([this](auto snapshot) { this->process_object(std::move(snapshot)); }, snapshot);
  }

private:
  void process_object(engine_snapshot::OrderAccepted snapshot) { report_generator_sink_.push(snapshot); }

  void process_object(engine_snapshot::Trade snapshot) {
    market_data_sink_.push(market_data_snapshot::Trade{
        .security_id = snapshot.security_id,
        .price = snapshot.price,
        .quantity = snapshot.quantity,
        .trade_id = snapshot.trade_id,
        .aggressor_order_id = snapshot.aggressor_order_id,
        .recipient_order_id = snapshot.recipient_order_id,
    });
    report_generator_sink_.push(report::Trade{
        .security_id = snapshot.security_id,
        .aggressor_direction = snapshot.aggressor_direction,
        .price = snapshot.price,
        .quantity = snapshot.quantity,
        .client_id = snapshot.aggressor_client_id,
        .client_order_id = snapshot.aggressor_client_order_id,
        .trade_id = snapshot.trade_id,
        .aggressor_order_id = snapshot.aggressor_order_id,
        .recipient_order_id = snapshot.recipient_order_id,
    });
    report_generator_sink_.push(report::Trade{
        .security_id = snapshot.security_id,
        .aggressor_direction = snapshot.aggressor_direction,
        .price = snapshot.price,
        .quantity = snapshot.quantity,
        .client_id = snapshot.recipient_client_id,
        .client_order_id = snapshot.recipient_client_order_id,
        .trade_id = snapshot.trade_id,
        .aggressor_order_id = snapshot.aggressor_order_id,
        .recipient_order_id = snapshot.recipient_order_id,
    });
  }

  void process_object(engine_snapshot::OrderBookEntry snapshot) {
    market_data_sink_.push(market_data_snapshot::BookOrderEntry{
        .order_id = snapshot.order_id,
        .security_id = snapshot.security_id,
        .direction = snapshot.direction,
        .price = snapshot.price,
        .quantity = snapshot.quantity,
    });
    report_generator_sink_.push(snapshot);
  }

  void process_object(engine_snapshot::OrderFilled snapshot) {
    if (snapshot.type == engine_snapshot::FilledType::PASSIVE_FILL) {
      if (snapshot.fully_filled) {
        market_data_sink_.push(market_data_snapshot::BookOrderDeletion{
            .order_id = snapshot.order_id,
            .security_id = snapshot.security_id,
        });
      } else {
        market_data_sink_.push(market_data_snapshot::BookOrderChange{
            .order_id = snapshot.order_id,
            .security_id = snapshot.security_id,
            .direction = snapshot.direction,
            .price = snapshot.price,
            .new_quantity = snapshot.remaining_quantity,
        });
      }
    }
    if (snapshot.fully_filled) {
      report_generator_sink_.push(snapshot);
    }
  }

  void process_object(engine_snapshot::OrderExpired snapshot) { report_generator_sink_.push(snapshot); }

  void process_object(engine_snapshot::OrderCancelled snapshot) {
    market_data_sink_.push(market_data_snapshot::BookOrderDeletion{
        .order_id = snapshot.order_id,
        .security_id = snapshot.security_id,
    });
    report_generator_sink_.push(snapshot);
  }

  void process_object(engine_snapshot::OrderRejected snapshot) { report_generator_sink_.push(snapshot); }

  void process_object(engine_snapshot::CancellationRejected snapshot) { report_generator_sink_.push(snapshot); }

  void process_object(engine_snapshot::ReplacementAccepted snapshot) {
    market_data_sink_.push(market_data_snapshot::BookOrderDeletion{
        .order_id = snapshot.order_id,
        .security_id = snapshot.security_id,
    });
    report_generator_sink_.push(snapshot);
  }

  void process_object(engine_snapshot::ReplacementRejected snapshot) { report_generator_sink_.push(snapshot); }

  MDSinkType<market_data_snapshot::SnapshotType>& market_data_sink_;
  ORSinkType<report::ReportType>& report_generator_sink_;
};

}  // namespace exchange_sim