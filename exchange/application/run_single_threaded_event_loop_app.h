#pragma once
#include "base/allocators/allocator_tracking.h"
#include "base/file.h"
#include "base/threaded_journal.h"
#include "base/threading/runtime.h"

#include "domain/generate_securities.h"
#include "domain/security_storage.h"

#include "exchange/domain/engine_events.h"
#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"
#include "exchange/input/order_generator_concept.h"
#include "exchange/matching/matching_engine_concept.h"
#include "exchange/output/dispatcher_concept.h"

#include <boost/container/static_vector.hpp>
#include <glaze/glaze.hpp>

#include <memory>

namespace exchange_sim {

namespace event_loop {

template<typename T>
class WrapVector {
public:
  explicit WrapVector(auto&&... args): vector_{std::forward<decltype(args)>(args)...} {}

  void push(auto&&... args) { vector_.push_back(std::forward<decltype(args)>(args)...); }

  auto begin(this auto&& self) { return std::forward_like<decltype(self)>(self.vector_).begin(); }

  auto end(this auto&& self) { return std::forward_like<decltype(self)>(self.vector_).end(); }

  void clear() { vector_.clear(); }

private:
  T vector_;
};

struct Sinks {
  static constexpr size_t constQueueSize = 1024;

  using RequestSinkType = WrapVector<boost::container::static_vector<request::RequestType, constQueueSize>>;
  using EngineEventSinkType = WrapVector<boost::container::static_vector<engine_events::EventType, constQueueSize>>;
  using MarketDataSinkType = WrapVector<boost::container::static_vector<market_events::EventType, constQueueSize>>;
  using ReportSinkType = WrapVector<boost::container::static_vector<report::ReportType, constQueueSize>>;

  static constexpr size_t constPage = 4096;
  static constexpr size_t constGuard = constPage / 3 - ((constPage / 3) % 64);

  RequestSinkType request_sink;
  std::array<std::byte, constGuard> padding1{};
  EngineEventSinkType engine_event_sink;
  std::array<std::byte, constGuard> padding2{};
  MarketDataSinkType market_data_generator_sink;
  std::array<std::byte, constGuard> padding3{};
  ReportSinkType report_generator_sink;

  Sinks() {
    static constexpr auto far_from_page = [](size_t offset1, size_t offset2) {
      size_t r = (offset2 - offset1) % constPage;
      return r >= constGuard && r <= constPage - constGuard;
    };

    static constexpr size_t constRequestSinkDelta = 0;
    static constexpr size_t constEngineSinkDelta = constRequestSinkDelta + sizeof(request_sink) + sizeof(padding1);
    static constexpr size_t constMarketDataSinkDelta =
        constEngineSinkDelta + sizeof(engine_event_sink) + sizeof(padding2);
    static constexpr size_t constReportSinkDelta =
        constMarketDataSinkDelta + sizeof(market_data_generator_sink) + sizeof(padding3);

    static_assert(far_from_page(constRequestSinkDelta, constEngineSinkDelta));
    static_assert(far_from_page(constEngineSinkDelta, constMarketDataSinkDelta));
    static_assert(far_from_page(constMarketDataSinkDelta, constReportSinkDelta));

    static_assert(sizeof(Sinks) ==
        sizeof(request_sink) + sizeof(padding1) + sizeof(engine_event_sink) + sizeof(padding2) +
            sizeof(market_data_generator_sink) + sizeof(padding3) + sizeof(report_generator_sink));
  }
};

}  // namespace event_loop

template<template<typename> typename OrderGenerator,
    template<typename> typename MatchingEngine,
    template<typename, typename> typename ExchangeDispatcher>
void run_single_threaded_event_loop_app(const std::shared_ptr<Runtime>& runtime, const glz::generic& config) {
  auto dedicated_core = runtime->core_allocator().lease_core(CoreType::DEDICATED);
  dedicated_core.pin_thread("run_single_threaded_event_loop_app");

  SecurityStorage security_storage(generate_securities(config));

  static constexpr size_t constJournalSize = 1 << 20;

  ThreadedJournal<File, market_events::EventType> market_data_journal("Journal<market_events::EventType>",
      config["journaling"]["market_events"].as<std::string>(),
      decltype(market_data_journal)::Mode::WRITE,
      constJournalSize,
      runtime->core_allocator().lease_core(CoreType::SHARED));
  ThreadedJournal<File, report::ReportType> report_journal("Journal<report::ReportType>",
      config["journaling"]["reports"].as<std::string>(),
      decltype(report_journal)::Mode::WRITE,
      constJournalSize,
      runtime->core_allocator().lease_core(CoreType::SHARED));
  ThreadedJournal<File, request::RequestType> request_journal("Journal<request::RequestType>",
      config["journaling"]["requests"].as<std::string>(),
      decltype(request_journal)::Mode::WRITE,
      constJournalSize,
      runtime->core_allocator().lease_core(CoreType::SHARED));

  auto stop_token = runtime->stop_token();

  auto sinks = std::make_unique<event_loop::Sinks>();

  using AppOrderGenerator = OrderGenerator<decltype(sinks->request_sink)>;

  static_assert(concepts::OrderGenerator<AppOrderGenerator>);

  AppOrderGenerator order_generator_wrapper(runtime,
      security_storage,
      request_journal,
      market_data_journal,
      report_journal,
      sinks->request_sink,
      config["order_generator"]);

  using AppMatchingEngine = MatchingEngine<event_loop::Sinks::EngineEventSinkType>;

  static_assert(concepts::MatchingEngine<AppMatchingEngine>);

  AppMatchingEngine matching_engine(
      config["matching_engine"], security_storage, std::nullopt, sinks->engine_event_sink);

  using AppExchangeDispatcher =
      ExchangeDispatcher<decltype(sinks->market_data_generator_sink), decltype(sinks->report_generator_sink)>;

  static_assert(concepts::ExchangeDispatcher<AppExchangeDispatcher>);

  AppExchangeDispatcher event_dispatcher(
      config["exchange_dispatcher"], sinks->market_data_generator_sink, sinks->report_generator_sink);

  AllocatorTracking::instance().enable_tracking();

  while (!stop_token.stop_requested() && !order_generator_wrapper.generator_stopped()) {
    order_generator_wrapper.generate_event();
    for (const auto& request : sinks->request_sink) {
      matching_engine.process(request);
    }
    for (const auto& engine_event : sinks->engine_event_sink) {
      event_dispatcher.process(engine_event);
    }
    for (const auto& market_data_event : sinks->market_data_generator_sink) {
      order_generator_wrapper.process(market_data_event);
    }
    for (const auto& report_event : sinks->report_generator_sink) {
      order_generator_wrapper.process(report_event);
    }
    sinks->request_sink.clear();
    sinks->engine_event_sink.clear();
    sinks->market_data_generator_sink.clear();
    sinks->report_generator_sink.clear();
  }

  AllocatorTracking::instance().disable_tracking();
}


}  // namespace exchange_sim
