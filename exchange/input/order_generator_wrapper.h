#pragma once

#include "base/counter.h"

#include "domain/security_storage.h"

#include "exchange/domain/market_data_snapshots.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"
#include "exchange/input/order_storage.h"

#include <boost/hof/match.hpp>

namespace exchange_sim {

template<typename SinkType>
class GeneratorInterface {
public:
  explicit GeneratorInterface(SecurityStorage& security_storage, SinkType& sink):
      security_storage_(security_storage), active_orders_(security_storage.size()), sink_(sink) {}

  ClientOrderID send_order_add_request(ClientID client_id,
      SecurityID security_id,
      OrderTimeInForce time_in_force,
      OrderType order_type,
      Direction direction,
      Price price,
      Amount quantity) {
    auto client_order_id = generate_client_order_id();
    auto request = request::OrderAddRequest{
        .client_id = client_id,
        .client_order_id = client_order_id,
        .security_id = security_id,
        .time_in_force = time_in_force,
        .order_type = order_type,
        .direction = direction,
        .price = price,
        .quantity = quantity,
    };
    if (security_storage_.has_security(security_id)) {
      active_orders_[security_id].handle_order_sent(request);
    }
    sink_.push(std::move(request));
    return client_order_id;
  }

  ClientOrderID send_order_cancel_request(ClientID client_id, ClientOrderID client_order_id, SecurityID security_id) {
    auto client_cancellation_id = generate_client_order_id();
    auto request = request::OrderCancelRequest{.client_id = client_id,
        .client_order_id = client_order_id,
        .client_cancellation_id = client_cancellation_id,
        .security_id = security_id};
    if (security_storage_.has_security(security_id)) {
      active_orders_[security_id].handle_cancel_sent(request);
    }
    sink_.push(std::move(request));
    return client_cancellation_id;
  }

  ClientOrderID send_order_replace_request(ClientID client_id,
      ClientOrderID client_order_id,
      SecurityID security_id,
      Direction direction,
      Price price,
      Amount quantity) {
    ClientOrderID client_replacement_id = generate_client_order_id();
    auto request = request::OrderReplaceRequest{
        .client_id = client_id,
        .client_order_id = client_order_id,
        .client_replacement_id = client_replacement_id,
        .security_id = security_id,
        .direction = direction,
        .price = price,
        .quantity = quantity,
    };
    if (security_storage_.has_security(security_id)) {
      active_orders_[security_id].handle_replace_sent(request);
    }
    sink_.push(std::move(request));
    return client_replacement_id;
  }

  void handle(report::ReportType report) {
    std::visit(boost::hof::match(
                   [this](const report::OrderAccepted& report) {
                     active_orders_[report.security_id].handle_order_accepted(report);
                   },
                   [](const report::Trade&) {},
                   [this](const report::OrderBookEntry& report) {
                     active_orders_[report.security_id].handle_order_book_entry(report);
                   },
                   [this](const report::OrderFilled& report) {
                     active_orders_[report.security_id].handle_order_filled(report);
                   },
                   [this](const report::OrderExpired& report) {
                     active_orders_[report.security_id].handle_order_expired(report);
                   },
                   [this](const report::OrderCancelled& report) {
                     active_orders_[report.security_id].handle_order_canceled(report);
                   },
                   [this](const report::OrderRejected& report) {
                     active_orders_[report.security_id].handle_order_rejected(report);
                   },
                   [this](const report::CancellationRejected& report) {
                     active_orders_[report.security_id].handle_cancel_rejected(report);
                   },
                   [this](const report::ReplacementAccepted& report) {
                     active_orders_[report.security_id].handle_replace_accepted(report);
                   },
                   [this](const report::ReplacementRejected& report) {
                     active_orders_[report.security_id].handle_replace_rejected(report);
                   }),
        report);
  }

  const auto& security_storage() const { return security_storage_; }

  const auto& active_orders() const { return active_orders_; }

  bool asked_to_stop() const { return false; }

private:
  ClientOrderID generate_client_order_id() { return current_client_order_id_.inc(); }

  SecurityStorage& security_storage_;
  StrongTypeIndexedVector<SecurityID, OrderStorage> active_orders_;
  Counter<ClientOrderID> current_client_order_id_;
  SinkType sink_;
};

template<template<typename> typename OrderGenerator, template<typename> typename GeneratorInterface, typename SinkType>
class OrderGeneratorWrapper {
public:
  template<typename... Args>
  explicit OrderGeneratorWrapper(SecurityStorage& security_storage, SinkType& sink, Args&&... args):
      generator_interface_(security_storage, sink), generator_(generator_interface_, std::forward<Args>(args)...) {}

  void process(market_data_snapshot::SnapshotType&& market_data_snapshot) {
    generator_.process(std::move(market_data_snapshot));
  }

  void process(report::ReportType&& report) {
    generator_interface_.handle(report);
    generator_.process(std::move(report));
  }

  void run() { generator_.run(); }

private:
  GeneratorInterface<SinkType> generator_interface_;
  OrderGenerator<GeneratorInterface<SinkType>> generator_;
};

}  // namespace exchange_sim