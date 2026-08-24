#include "base/callback_receiver.h"
#include "base/journal.h"
#include "base/run.h"
#include "base/thread_receiver.h"

#include "domain/generate_securities.h"
#include "domain/security_storage.h"

#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"
#include "exchange/input/order_generator_wrapper.h"
#include "exchange/input/simple_order_generator.h"
#include "exchange/matching/matching_engine.h"
#include "exchange/matching/stl_book_container.h"
#include "exchange/output/exchange_dispatcher.h"

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>
#include <quill/Backend.h>

#include <iostream>

namespace exchange_sim {

namespace {

void run_multi_threaded_event_loop_app(const std::shared_ptr<Runtime>& runtime, const glz::generic& config) {
  auto dedicated_core = runtime->core_allocator().lease_core(CoreType::DEDICATED);
  dedicated_core.pin_thread("run_multi_threaded_app");

  SecurityStorage security_storage(generate_securities(config));

  static constexpr size_t constJournalSize = 1 << 20;

  Journal<market_events::EventType> market_data_journal("Journal<market_events::EventType>",
      config["journaling"]["market_events"].as<std::string>(),
      JournalMode::WRITE,
      runtime->core_allocator().lease_core(CoreType::SHARED),
      constJournalSize);
  Journal<report::ReportType> report_journal("Journal<report::ReportType>",
      config["journaling"]["reports"].as<std::string>(),
      JournalMode::WRITE,
      runtime->core_allocator().lease_core(CoreType::SHARED),
      constJournalSize);
  Journal<request::RequestType> request_journal("Journal<request::RequestType>",
      config["journaling"]["requests"].as<std::string>(),
      JournalMode::WRITE,
      runtime->core_allocator().lease_core(CoreType::SHARED),
      constJournalSize);

  static constexpr size_t constQueueSize = 1 << 14;

  auto stop_token = runtime->stop_token();

  using MarketDataSinkType = SPSCQueue<market_events::EventType>;
  using ReportSinkType = SPSCQueue<report::ReportType>;

  MarketDataSinkType market_data_generator_sink(constQueueSize);
  ReportSinkType report_generator_sink(constQueueSize);

  CallbackReceiver<engine_events::EventType, ExchangeDispatcher<MarketDataSinkType, ReportSinkType>> exchange_dispatcher(
      config["exchange_dispatcher"],
      market_data_generator_sink,
      report_generator_sink);

  using ExchangeDispatcherSinkType = std::remove_cvref_t<decltype(exchange_dispatcher.get_sink())>;

  ThreadReceiver<request::RequestType, MatchingEngine<StlL3BookContainer, ExchangeDispatcherSinkType>> matching_engine(
      "MatchingEngine",
      constQueueSize,
      runtime->core_allocator().lease_core(CoreType::DEDICATED),
      config["matching_engine"],
      security_storage,
      exchange_dispatcher.get_sink());

  using MatchingEngineSinkType = std::remove_cvref_t<decltype(matching_engine.get_sink())>;

  OrderGeneratorWrapper<SimpleOrderGenerator, GeneratorInterface, MatchingEngineSinkType> order_generator(
      runtime->stop_token(),
      security_storage,
      request_journal,
      market_data_journal,
      report_journal,
      matching_engine.get_sink(),
      config["order_generator"]);

  while (!matching_engine.ready_to_receive()) {
    std::this_thread::sleep_for(100ms);
  }

  auto consumer = [&](auto&& event) {
    return order_generator.process(std::forward<decltype(event)>(event));
  };

  auto consume = [&] {
    return market_data_generator_sink.consume(consumer) || report_generator_sink.consume(consumer);
  };

  while (!stop_token.stop_requested()) {
    if (!consume()) {
      order_generator.generate_event();
    }
  }

  matching_engine.ask_to_stop();

  Ticks current = Ticks::rdtsc();
  while (current.nanoseconds_till_now() < 100ms) {
    if (consume()) {
      current = Ticks::rdtsc();
    }
  }

  matching_engine.stop();
  while (consume()) {
  }

  CHECK(market_data_generator_sink.empty(), "MarketDataSink must be empty");
  CHECK(report_generator_sink.empty(), "ReportGeneratorSink must be empty");
  CHECK(matching_engine.get_sink().empty(), "MatchingEngineSink must be empty");
}

}  // namespace

}  // namespace exchange_sim


int main(int argc, char* argv[]) {
  int exit_code = 0;
  CPPTRACE_TRY {
    const auto [runtime, config] =
        exchange_sim::read_config_and_initialize_logging(std::span(argv, argv + argc), false);
    exchange_sim::run_multi_threaded_event_loop_app(runtime, config);
    quill::Backend::stop();
  }
  CPPTRACE_CATCH(...) {
    try {
      throw;
    } catch (const std::exception& e) {
      std::cerr << "Exception occurred: " << e.what() << '\n';
      exit_code = 1;
    } catch (...) {
      std::cerr << "Unknown exception occurred" << '\n';
      exit_code = 1;
    }
    std::cerr << "\nStacktrace:\n";
    cpptrace::from_current_exception().print_with_snippets();
  }
  return exit_code;
}