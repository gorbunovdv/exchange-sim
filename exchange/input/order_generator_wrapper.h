#pragma once

#include "base/counter.h"
#include "base/journal.h"
#include "base/throughput.h"

#include "domain/security_storage.h"

#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"

#include <utility>

namespace exchange_sim {

template<template<typename> typename OrderGenerator, typename GeneratorInterface, typename SinkType>
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
      generator_interface_(std::move(stop_token), security_storage, requests_journal, sink),
      generator_(generator_interface_, std::forward<Args>(args)...),
      market_data_journal_(market_data_journal),
      report_journal_(report_journal) {}

  ~OrderGeneratorWrapper() {
    CPPTRACE_TRY {
      apply_stored_events();
    }
    CPPTRACE_CATCH(...) {
      try {
        try {
          throw;
        } catch (const std::exception& e) {
          ERROR("Exception occurred: {}", e.what());
        } catch (...) {
          ERROR("Unknown exception occurred");
        }
        ERROR("\nStacktrace:");
        ERROR("{}", cpptrace::from_current_exception().to_string());
      } catch (...) {
        std::cerr << "Logger threw during handling another exception" << '\n';
      }
    }
  }

  void process(const market_events::EventType& market_event) {
    auto timestamp = Ticks::rdtscp();
    event_buffer_.emplace_back(market_event, timestamp);
  }

  void process(const report::ReportType& report) {
    auto timestamp = Ticks::rdtscp();
    report_buffer_.emplace_back(report, timestamp);
  }

  void run() {
    while (!generator_interface_.asked_to_stop()) {
      apply_stored_events();
      generator_.generate_event();
    }
  }

  void generate_event() {
    if (!generator_interface_.asked_to_stop()) {
      apply_stored_events();
      generator_.generate_event();
    }
  }

private:
  void apply_stored_events() {
    generator_interface_.apply_stored_events();
    for (const auto& [market_event, timestamp] : event_buffer_) {
      market_data_journal_.write(market_event, timestamp);
      generator_interface_.handle_market_event(market_event);
      generator_.process_market_event(market_event);
    }
    event_buffer_.clear();
    for (const auto& [report, timestamp] : report_buffer_) {
      report_journal_.write(report, timestamp);
      generator_interface_.handle_report(report);
      generator_.process_report(report);
    }
    report_buffer_.clear();
  }

  GeneratorInterface generator_interface_;
  OrderGenerator<GeneratorInterface> generator_;

  Journal<market_events::EventType>& market_data_journal_;
  Journal<report::ReportType>& report_journal_;

  std::vector<Timestamped<market_events::EventType>> event_buffer_;
  std::vector<Timestamped<report::ReportType>> report_buffer_;
};

}  // namespace exchange_sim
