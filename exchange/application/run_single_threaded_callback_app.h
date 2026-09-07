#pragma once
#include "base/callback_receiver.h"
#include "base/callback_sink.h"
#include "base/forward_sink.h"
#include "base/threading/runtime.h"

#include "domain/generate_securities.h"
#include "domain/security_storage.h"

#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"
#include "exchange/input/order_generator_wrapper.h"

#include <glaze/glaze.hpp>

namespace exchange_sim {

template<template<typename> typename OrderGenerator,
    template<typename> typename MatchingEngine,
    template<typename, typename> typename ExchangeDispatcher>
void run_single_threaded_callback_app(const std::shared_ptr<Runtime>& runtime, const glz::generic& config) {
  auto dedicated_core = runtime->core_allocator().lease_core(CoreType::DEDICATED);
  dedicated_core.pin_thread("run_single_threaded_callback_app");

  SecurityStorage security_storage(generate_securities(config));

  static constexpr size_t constJournalSize = 1 << 20;

  Journal<market_events::EventType> market_data_journal("Journal<market_events::EventType>",
      config["journaling"]["market_events"].as<std::string>(),
      JournalMode::WRITE,
      constJournalSize,
      runtime->core_allocator().lease_core(CoreType::SHARED));
  Journal<report::ReportType> report_journal("Journal<report::ReportType>",
      config["journaling"]["reports"].as<std::string>(),
      JournalMode::WRITE,
      constJournalSize,
      runtime->core_allocator().lease_core(CoreType::SHARED));
  Journal<request::RequestType> request_journal("Journal<request::RequestType>",
      config["journaling"]["requests"].as<std::string>(),
      JournalMode::WRITE,
      constJournalSize,
      runtime->core_allocator().lease_core(CoreType::SHARED));

  using MarketDataInternalSinkType =
      CallbackSink<market_events::EventType, std::function<void(market_events::EventType&&)>>;
  using MarketDataSinkType = ForwardSink<market_events::EventType, MarketDataInternalSinkType>;
  using ReportInternalSinkType = CallbackSink<report::ReportType, std::function<void(report::ReportType&&)>>;
  using ReportSinkType = ForwardSink<report::ReportType, ReportInternalSinkType>;

  MarketDataSinkType market_data_generator_sink;
  ReportSinkType report_generator_sink;

  CallbackReceiver<engine_events::EventType, ExchangeDispatcher<MarketDataSinkType, ReportSinkType>>
      exchange_dispatcher(config["exchange_dispatcher"], market_data_generator_sink, report_generator_sink);

  using ExchangeDispatcherSinkType = std::remove_cvref_t<decltype(exchange_dispatcher.get_sink())>;

  CallbackReceiver<request::RequestType, MatchingEngine<ExchangeDispatcherSinkType>> matching_engine(
      config["matching_engine"], security_storage, exchange_dispatcher.get_sink());

  auto stop_token = runtime->stop_token();

  OrderGenerator order_generator(stop_token,
      security_storage,
      request_journal,
      market_data_journal,
      report_journal,
      matching_engine.get_sink(),
      config["order_generator"]);

  MarketDataInternalSinkType market_data_generator_sink_callback(
      [&order_generator](market_events::EventType&& snapshot) { order_generator.process(snapshot); });
  ReportInternalSinkType report_generator_sink_callback(
      [&order_generator](report::ReportType&& report) { order_generator.process(report); });

  market_data_generator_sink.set_sink(&market_data_generator_sink_callback);
  report_generator_sink.set_sink(&report_generator_sink_callback);

  while (!stop_token.stop_requested() && !order_generator.generator_stopped()) {
    order_generator.generate_event();
  }
}

}  // namespace exchange_sim