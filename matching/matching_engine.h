#pragma once

#include "base/counter.h"
#include "base/logging/loggable_class.h"
#include "domain/requests.h"
#include "domain/engine_snapshots.h"
#include "domain/security_storage.h"
#include "matching/simple_l3book.h"

#include <glaze/glaze.hpp>

namespace exchange_sim {

template<template<typename> class SinkType>
class MatchingEngine : public LoggableClass {
public:
  explicit MatchingEngine(
      const glz::generic&, SecurityStorage& security_storage, SinkType<engine_snapshot::SnapshotType>& sink):
      LoggableClass("MatchingEngine"), security_storage_(security_storage), sink_(sink) {
    books_.reserve(security_storage_.size());
    for (auto& security : security_storage_.securities()) {
      books_.emplace_back(security, order_id_counter_, trade_id_counter_);
    }
  }

  void process(request::RequestType request) {
    std::visit([this](auto request) { this->process_object(std::move(request)); }, request);
  }

private:
  void process_object(request::OrderAddRequest request) { books_[request.security_id].add(std::move(request), sink_); }

  void process_object(request::OrderReplaceRequest request) {
    books_[request.security_id].replace(std::move(request), sink_);
  }

  void process_object(request::OrderCancelRequest request) {
    books_[request.security_id].cancel(std::move(request), sink_);
  }

  SecurityStorage& security_storage_;
  SinkType<engine_snapshot::SnapshotType>& sink_;
  std::vector<SimpleL3Book> books_;

  Counter<ExchangeOrderID> order_id_counter_;
  Counter<ExchangeTradeID> trade_id_counter_;
};

}  // namespace exchange_sim