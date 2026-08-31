#pragma once

#include "base/counter.h"
#include "base/logging/loggable_class.h"

#include "domain/security_storage.h"

#include "exchange/domain/engine_events.h"
#include "exchange/domain/requests.h"
#include "exchange/matching/l3book.h"

#include <glaze/glaze.hpp>

namespace exchange_sim {

template<template<typename> class BookContainer, engine_events::Sink SinkType>
class MatchingEngine : public LoggableClass {
public:
  explicit MatchingEngine(const glz::generic&, const SecurityStorage& security_storage, SinkType& sink):
      LoggableClass("MatchingEngine"), security_storage_(security_storage), sink_(sink) {
    static_assert(request::Processor<MatchingEngine>);

    books_.reserve(security_storage_.size());
    for (auto& security : security_storage_.securities()) {
      books_.emplace_back(security, order_id_counter_, trade_id_counter_);
    }
  }

  void process(const request::RequestType& request) {
    std::visit([this](const auto& request) { this->process_object(request); }, request);
  }

  const auto& books() const { return books_; }

private:
  void process_object(const request::OrderAddRequest& request) {
    if (!security_storage_.has_security(request.security_id)) {
      sink_.push(engine_events::OrderRejected{
          .origin_time = request.origin_time,
          .security_id = request.security_id,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .reason = engine_events::OrderRejectedReason::SECURITY_NOT_FOUND,
      });
      return;
    }
    books_[request.security_id].add(request, sink_);
  }

  void process_object(const request::OrderReplaceRequest& request) {
    if (!security_storage_.has_security(request.security_id)) {
      sink_.push(engine_events::ReplacementRejected{
          .origin_time = request.origin_time,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .client_replacement_id = request.client_replacement_id,
          .security_id = request.security_id,
          .reason = engine_events::ReplacementRejectedReason::SECURITY_NOT_FOUND,
      });
      return;
    }
    books_[request.security_id].replace(request, sink_);
  }

  void process_object(const request::OrderCancelRequest& request) {
    if (!security_storage_.has_security(request.security_id)) {
      sink_.push(engine_events::CancellationRejected{
          .origin_time = request.origin_time,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .client_cancellation_id = request.client_cancellation_id,
          .security_id = request.security_id,
          .reason = engine_events::CancellationRejectedReason::SECURITY_NOT_FOUND,
      });
      return;
    }
    books_[request.security_id].cancel(request, sink_);
  }

  const SecurityStorage& security_storage_;
  SinkType& sink_;
  StrongTypeIndexedVector<SecurityID, L3Book<BookContainer>> books_;

  Counter<ExchangeOrderID> order_id_counter_;
  Counter<ExchangeTradeID> trade_id_counter_;
};

}  // namespace exchange_sim
