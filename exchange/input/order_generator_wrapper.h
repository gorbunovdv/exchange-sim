#pragma once

#include "base/counter.h"
#include "base/file.h"
#include "base/threaded_journal.h"
#include "base/threading/runtime.h"
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
  explicit OrderGeneratorWrapper(const std::shared_ptr<Runtime>& runtime,
      SecurityStorage& security_storage,
      ThreadedJournal<File, request::RequestType>& requests_journal,
      ThreadedJournal<File, market_events::EventType>& market_data_journal,
      ThreadedJournal<File, report::ReportType>& report_journal,
      SinkType& sink,
      Args&&... args):
      generator_interface_(runtime->stop_token(), runtime->core_allocator(), security_storage, requests_journal, sink),
      generator_(generator_interface_, std::forward<Args>(args)...),
      market_data_journal_(market_data_journal),
      report_journal_(report_journal) {
    event_buffer_.reserve(constEventBufferSize);
    report_buffer_.reserve(constEventBufferSize);
  }

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
    CHECK(event_buffer_.size() < event_buffer_.capacity(),
        "Event buffer is full, size={} capacity={}",
        event_buffer_.size(),
        event_buffer_.capacity());
    event_buffer_.emplace_back(market_event, timestamp);
  }

  void process(const report::ReportType& report) {
    auto timestamp = Ticks::rdtscp();
    CHECK(report_buffer_.size() < report_buffer_.capacity(),
        "Report buffer is full, size={} capacity={}",
        report_buffer_.size(),
        report_buffer_.capacity());
    report_buffer_.emplace_back(report, timestamp);
  }

  void generate_event() {
    if (!generator_interface_.asked_to_stop()) {
      apply_stored_events();
      generator_.generate_event();
    }
  }

  bool generator_stopped() const { return generator_interface_.generator_stopped(); }

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

  ThreadedJournal<File, market_events::EventType>& market_data_journal_;
  ThreadedJournal<File, report::ReportType>& report_journal_;

  static constexpr size_t constEventBufferSize = 10240;

  std::vector<Timestamped<market_events::EventType>> event_buffer_;
  std::vector<Timestamped<report::ReportType>> report_buffer_;
};

}  // namespace exchange_sim
