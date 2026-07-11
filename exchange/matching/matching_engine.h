#pragma once

#include "base/counter.h"
#include "base/logging/loggable_class.h"

#include "domain/security_storage.h"

#include "exchange/domain/engine_events.h"
#include "exchange/domain/requests.h"
#include "exchange/matching/simple_l3book.h"

#include <glaze/glaze.hpp>

namespace exchange_sim {

template<engine_events::Sink SinkType>
class MatchingEngine : public LoggableClass {
public:
  explicit MatchingEngine(const glz::generic&, SecurityStorage& security_storage, SinkType& sink):
      LoggableClass("MatchingEngine"), security_storage_(security_storage), sink_(sink) {
    static_assert(request::Processor<MatchingEngine>);

    books_.reserve(security_storage_.size());
    for (auto& security : security_storage_.securities()) {
      books_.emplace_back(security, order_id_counter_, trade_id_counter_);
    }
  }

  void process(request::RequestType&& request) {
    std::visit(
        [this](auto&& request) { this->process_object(std::forward<decltype(request)>(request)); }, std::move(request));
  }

private:
  void process_object(request::OrderAddRequest&& request) {
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
    books_[request.security_id].add(std::move(request), sink_);
  }

  void process_object(request::OrderReplaceRequest&& request) {
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
    books_[request.security_id].replace(std::move(request), sink_);
  }

  void process_object(request::OrderCancelRequest&& request) {
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
    books_[request.security_id].cancel(std::move(request), sink_);
  }

  SecurityStorage& security_storage_;
  SinkType& sink_;
  StrongTypeIndexedVector<SecurityID, SimpleL3Book> books_;

  Counter<ExchangeOrderID> order_id_counter_;
  Counter<ExchangeTradeID> trade_id_counter_;
};

}  // namespace exchange_sim