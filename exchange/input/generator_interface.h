#pragma once

#include "base/counter.h"
#include "base/journal.h"

#include "exchange/domain/market_events.h"
#include "exchange/input/order_storage.h"

#include <boost/hof/match.hpp>

namespace exchange_sim {

template<typename SinkType, typename... Skills>
class GeneratorInterface : public Skills... {
public:
  explicit GeneratorInterface(std::stop_token stop_token,
      SecurityStorage& security_storage,
      Journal<request::RequestType>& journal,
      SinkType& sink):
      Skills(security_storage)...,
      stop_token_(std::move(stop_token)),
      security_storage_(security_storage),
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
    auto timestamp = Ticks::rdtscp();
    request.origin_time = timestamp;
    sink_.push(request);
    journal_.write(request, timestamp);
    (Skills::handle_request(request), ...);
    return client_order_id;
  }

  ClientOrderID send_order_cancel_request(ClientID client_id, ClientOrderID client_order_id, SecurityID security_id) {
    auto client_cancellation_id = generate_client_order_id();
    auto request = request::OrderCancelRequest{.origin_time = Ticks{0},
        .client_id = client_id,
        .client_order_id = client_order_id,
        .client_cancellation_id = client_cancellation_id,
        .security_id = security_id};
    auto timestamp = Ticks::rdtscp();
    request.origin_time = timestamp;
    sink_.push(request);
    journal_.write(request, timestamp);
    (Skills::handle_request(request), ...);
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
    auto timestamp = Ticks::rdtscp();
    request.origin_time = timestamp;
    sink_.push(request);
    journal_.write(request, timestamp);
    (Skills::handle_request(request), ...);
    return client_replacement_id;
  }

  void handle_report(const report::ReportType& report) {
    std::visit([this](const auto& report) { (Skills::handle_report(report), ...); }, report);
  }

  void handle_market_event(const market_events::EventType& event) {
    std::visit([this](const auto& market_event) { (Skills::handle_market_event(market_event), ...); }, event);
  }

  const auto& security_storage() const { return security_storage_; }

  bool asked_to_stop() const { return stop_token_.stop_requested(); }

private:
  ClientOrderID generate_client_order_id() { return current_client_order_id_.inc(); }

  std::stop_token stop_token_;
  SecurityStorage& security_storage_;
  Counter<ClientOrderID> current_client_order_id_;
  SinkType& sink_;
  Journal<request::RequestType>& journal_;
};

}  // namespace exchange_sim