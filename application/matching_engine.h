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

  void process(RequestType) {}

private:
  MDSinkType<MarketDataSnapshotType>& market_data_sink_;
  ORSinkType<OrderReportType>& report_generator_sink_;
};

}  // namespace exchange_sim