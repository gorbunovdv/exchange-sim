#pragma once
#include "base/file.h"
#include "base/mpsc_queue.h"
#include "base/thread_receiver.h"
#include "base/threaded_journal.h"
#include "base/threading/runtime.h"

#include "domain/generate_securities.h"
#include "domain/security_storage.h"

#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"
#include "exchange/input/order_generator_concept.h"
#include "exchange/matching/matching_engine_concept.h"
#include "exchange/output/dispatcher_concept.h"

#include <glaze/glaze.hpp>

#include <memory>

namespace exchange_sim {

template<typename ArgumentClass, typename BaseClass>
class MultiSectionReceiver {
public:
  explicit MultiSectionReceiver(std::string name,
      const SecurityStorage& storage,
      size_t queue_size,
      CoreAllocator& core_allocator,
      const glz::generic& config,
      engine_events::Sink auto& sink) {
    receivers_.reserve(storage.sections_count());
    for (uint32_t i = 0; i < storage.sections_count(); i++) {
      receivers_.emplace_back(
          std::make_unique<ThreadReceiver<ArgumentClass, BaseClass, SPSCQueue>>(std::format("{}_section_{}", name, i),
              queue_size,
              core_allocator.lease_core(CoreType::DEDICATED),
              config,
              storage,
              SectionID{i},
              sink));
    }
    section_by_security_id_.reserve(storage.size());
    for (const auto& security : storage.securities()) {
      section_by_security_id_.push_back(security.section_id);
    }
  }

  bool ready_to_receive() const {
    return std::ranges::all_of(receivers_, [](const auto& receiver) { return receiver->ready_to_receive(); });
  }

  void push(request::RequestType&& request) {
    SecurityID security_id = std::visit([](const auto& request) { return request.security_id; }, request);
    receivers_[section_by_security_id_[security_id]]->get_sink().push(std::move(request));
  }

  auto& get_sink() { return *this; }

  void ask_to_stop() {
    for (auto& receiver : receivers_) {
      receiver->ask_to_stop();
    }
  }

  void stop() {
    for (auto& receiver : receivers_) {
      receiver->stop();
    }
  }

  bool empty() const {
    return std::ranges::all_of(receivers_, [](auto& receiver) { return receiver->get_sink().empty(); });
  }

private:
  StrongTypeIndexedVector<SectionID, std::unique_ptr<ThreadReceiver<ArgumentClass, BaseClass, SPSCQueue>>> receivers_;
  StrongTypeIndexedVector<SecurityID, SectionID> section_by_security_id_;
};

template<template<typename> typename OrderGenerator,
    template<typename> typename MatchingEngine,
    template<typename, typename> typename ExchangeDispatcher>
void run_multi_threaded_app(const std::shared_ptr<Runtime>& runtime, const glz::generic& config) {
  auto dedicated_core = runtime->core_allocator().lease_core(CoreType::DEDICATED);
  dedicated_core.pin_thread("run_multi_threaded_app");

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

  static constexpr size_t constQueueSize = 1 << 14;

  auto stop_token = runtime->stop_token();

  using MarketDataSinkType = AlignedQueue<SPSCQueue, market_events::EventType>;
  using ReportSinkType = AlignedQueue<SPSCQueue, report::ReportType>;

  MarketDataSinkType market_data_generator_sink(constQueueSize);
  ReportSinkType report_generator_sink(constQueueSize);

  using AppDispatcher = ExchangeDispatcher<MarketDataSinkType, ReportSinkType>;

  static_assert(concepts::ExchangeDispatcher<AppDispatcher>);

  ThreadReceiver<engine_events::EventType, AppDispatcher, MPSCQueue> exchange_dispatcher("ExchangeDispatcher",
      constQueueSize,
      runtime->core_allocator().lease_core(CoreType::DEDICATED),
      config["exchange_dispatcher"],
      market_data_generator_sink,
      report_generator_sink);

  using ExchangeDispatcherSinkType = std::remove_cvref_t<decltype(exchange_dispatcher.get_sink())>;

  using AppMatchingEngine = MatchingEngine<ExchangeDispatcherSinkType>;

  static_assert(concepts::MatchingEngine<AppMatchingEngine>);

  MultiSectionReceiver<request::RequestType, AppMatchingEngine> matching_engines("MatchingEngine",
      security_storage,
      constQueueSize,
      runtime->core_allocator(),
      config["matching_engine"],
      exchange_dispatcher.get_sink());

  using AppOrderGenerator = OrderGenerator<decltype(matching_engines.get_sink())>;

  AppOrderGenerator order_generator(runtime,
      security_storage,
      request_journal,
      market_data_journal,
      report_journal,
      matching_engines.get_sink(),
      config["order_generator"]);

  static_assert(concepts::OrderGenerator<AppOrderGenerator>);

  while (!exchange_dispatcher.ready_to_receive()) {
    std::this_thread::sleep_for(100ms);
  }

  while (!matching_engines.ready_to_receive()) {
    std::this_thread::sleep_for(100ms);
  }

  auto consumer = [&](auto&& event) {
    return order_generator.process(std::forward<decltype(event)>(event));
  };

  auto consume = [&] {
    bool md_consume = market_data_generator_sink.consume(consumer);
    bool rt_consume = report_generator_sink.consume(consumer);
    return md_consume || rt_consume;
  };

  while (!stop_token.stop_requested() && !order_generator.generator_stopped()) {
    if (!order_generator.generate_event()) {
      consume();
    }
  }

  matching_engines.ask_to_stop();
  exchange_dispatcher.ask_to_stop();

  Ticks current = Ticks::rdtsc();
  while (current.nanoseconds_till_now() < 100ms) {
    if (consume()) {
      current = Ticks::rdtsc();
    }
  }

  matching_engines.stop();
  exchange_dispatcher.stop();
  while (consume()) {
  }

  CHECK(market_data_generator_sink.empty(), "MarketDataSink must be empty");
  CHECK(report_generator_sink.empty(), "ReportGeneratorSink must be empty");
  CHECK(matching_engines.get_sink().empty(), "MatchingEngineSink must be empty");
  CHECK(exchange_dispatcher.get_sink().empty(), "ExchangeDispatcherSink must be empty");
}

}  // namespace exchange_sim
