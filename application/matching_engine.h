#pragma once

#include "base/macros.h"
#include "market_data_generator.h"
#include "report_generator.h"

namespace exchange_sim {

template<template<typename> class MDSinkType, template<typename> class ORSinkType>
class MatchingEngine : public LoggableClass {
public:
  explicit MatchingEngine(const glz::generic&,
      MDSinkType<MarketDataSnapshotType>& market_data_sink,
      ORSinkType<OrderReportType>& report_generator_sink):
      LoggableClass("MatchingEngine"),
      market_data_sink_(market_data_sink),
      report_generator_sink_(report_generator_sink) {
    UNUSED(market_data_sink_);
    UNUSED(report_generator_sink_);
  }

  void process(RequestType request) {
    std::visit(
        boost::hof::match(
            [this](const OrderAddRequest& request) {
              INFO("OrderAddRequest received, client_id={} security_id={} quantity={} price={}",
                  request.client_id,
                  request.security_id,
                  static_cast<double>(request.quantity),
                  static_cast<double>(request.price));
            },
            [this](const OrderCancelRequest& request) {
              INFO("OrderCancelRequest received, client_id={} security_id={}", request.client_id, request.security_id);
            },
            [this](const OrderModifyRequest& request) {
              INFO("OrderModifyRequest received, client_id={} security_id={} quantity={} price={}",
                  request.client_id,
                  request.security_id,
                  static_cast<double>(request.quantity),
                  static_cast<double>(request.price));
            }),
        request);
  }

private:
  MDSinkType<MarketDataSnapshotType>& market_data_sink_;
  ORSinkType<OrderReportType>& report_generator_sink_;
};

}  // namespace exchange_sim