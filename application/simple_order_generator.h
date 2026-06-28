#pragma once

#include "base/logging/loggable_class.h"
#include "base/macros.h"
#include "application/events.h"

#include "base/time/rdtsc.h"

namespace exchange_sim {

template<template<typename> class SinkType>
class SimpleOrderGenerator : public LoggableClass {
public:
  explicit SimpleOrderGenerator(const glz::generic&, SinkType<RequestType>& matching_engine_sink):
      LoggableClass("SimpleOrderGenerator"), matching_engine_sink_(matching_engine_sink) {
    UNUSED(matching_engine_sink_);
  }

  void run() {}

private:
  class RandomGenerator {
  public:
  private:
  };

  SinkType<RequestType>& matching_engine_sink_;
};

}  // namespace exchange_sim