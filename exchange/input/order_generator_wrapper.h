#pragma once

#include "base/counter.h"
#include "base/throughput.h"

#include "domain/security_storage.h"

#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"
#include "exchange/input/order_storage.h"

#include <boost/hof/match.hpp>

#include <utility>

namespace exchange_sim {

template<typename SinkType>
class GeneratorInterface {
public:
  explicit GeneratorInterface(std::stop_token stop_token,
      SecurityStorage& security_storage,
      Journal<request::RequestType>& journal,
      SinkType& sink):
      stop_token_(std::move(stop_token)),
      security_storage_(security_storage),
      active_orders_(security_storage.size()),
      sink_(sink),
      journal_(journal) {}

  ClientOrderID send_order_add_request(ClientID client_id,
      SecurityID security_id,
      OrderTimeInForce time_in_force,
      OrderType order_type,
      Direction direction,
      Price price,
      Amount quantity) {
    auto client_order_id = generate_client_order_id();
    auto request = request::OrderAddRequest{
        .origin_time = Ticks{0},
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
      active_orders_[security_id].handle(request);
    }
    auto timestamp = Ticks::rdtscp();
    request.origin_time = timestamp;
    sink_.push(request);
    journal_.write(timestamp, std::move(request));
    return client_order_id;
  }

  ClientOrderID send_order_cancel_request(ClientID client_id, ClientOrderID client_order_id, SecurityID security_id) {
    auto client_cancellation_id = generate_client_order_id();
    auto request = request::OrderCancelRequest{.origin_time = Ticks{0},
        .client_id = client_id,
        .client_order_id = client_order_id,
        .client_cancellation_id = client_cancellation_id,
        .security_id = security_id};
    if (security_storage_.has_security(security_id)) {
      active_orders_[security_id].handle(request);
    }
    auto timestamp = Ticks::rdtscp();
    request.origin_time = timestamp;
    sink_.push(request);
    journal_.write(timestamp, std::move(request));
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
        .origin_time = Ticks{0},
        .client_id = client_id,
        .client_order_id = client_order_id,
        .client_replacement_id = client_replacement_id,
        .security_id = security_id,
        .direction = direction,
        .price = price,
        .quantity = quantity,
    };
    if (security_storage_.has_security(security_id)) {
      active_orders_[security_id].handle(request);
    }
    auto timestamp = Ticks::rdtscp();
    request.origin_time = timestamp;
    sink_.push(request);
    journal_.write(timestamp, std::move(request));
    return client_replacement_id;
  }

  void handle(report::ReportType report) {
    std::visit(boost::hof::match([](const report::Trade&) {},
                   [this](const auto& report) {
                     if (security_storage_.has_security(report.security_id)) {
                       active_orders_[report.security_id].handle(report);
                     }
                   }),
        report);
  }

  const auto& security_storage() const { return security_storage_; }

  const auto& active_orders() const { return active_orders_; }

  bool asked_to_stop() const { return stop_token_.stop_requested(); }

private:
  ClientOrderID generate_client_order_id() { return current_client_order_id_.inc(); }

  std::stop_token stop_token_;
  SecurityStorage& security_storage_;
  StrongTypeIndexedVector<SecurityID, OrderStorage> active_orders_;
  Counter<ClientOrderID> current_client_order_id_;
  SinkType& sink_;
  Journal<request::RequestType>& journal_;
};

template<template<typename> typename OrderGenerator, template<typename> typename GeneratorInterface, typename SinkType>
class OrderGeneratorWrapper {
public:
  template<typename... Args>
  explicit OrderGeneratorWrapper(std::stop_token stop_token,
      SecurityStorage& security_storage,
      Journal<request::RequestType>& requests_journal,
      Journal<market_events::EventType>& market_data_journal,
      Journal<report::ReportType>& report_journal,
      SinkType& sink,
      Args&&... args):
      generator_interface_(stop_token, security_storage, requests_journal, sink),
      generator_(generator_interface_, std::forward<Args>(args)...),
      market_data_journal_(market_data_journal),
      report_journal_(report_journal) {}

  void process(market_events::EventType&& market_event) {
    auto timestamp = Ticks::rdtscp();
    market_data_journal_.write(timestamp, market_event);
    generator_.process(std::move(market_event));
  }

  void process(report::ReportType&& report) {
    auto timestamp = Ticks::rdtscp();
    report_journal_.write(timestamp, report);
    generator_interface_.handle(report);
    generator_.process(std::move(report));
  }

  void run() {
    while (!generator_interface_.asked_to_stop()) {
      generator_.run();
    }
  }

private:
  GeneratorInterface<SinkType> generator_interface_;
  OrderGenerator<GeneratorInterface<SinkType>> generator_;

  Journal<market_events::EventType>& market_data_journal_;
  Journal<report::ReportType>& report_journal_;
};

}  // namespace exchange_sim