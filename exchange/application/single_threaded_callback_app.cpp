#include "base/callback_receiver.h"
#include "base/forward_sink.h"
#include "base/journal.h"
#include "base/logging/quill_sinks.h"
#include "base/run.h"

#include "domain/generate_securities.h"
#include "domain/security.h"

#include "exchange/domain/engine_events.h"
#include "exchange/input/generator_interface.h"
#include "exchange/input/order_generator_wrapper.h"
#include "exchange/input/simple_order_generator.h"
#include "exchange/input/store_orders_skill.h"
#include "exchange/matching/matching_engine.h"
#include "exchange/matching/stl_book_container.h"
#include "exchange/output/exchange_dispatcher.h"

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>
#include <quill/Backend.h>

#include <iostream>

namespace exchange_sim {

namespace {

void run_single_threaded_callback_app(const std::shared_ptr<Runtime>& runtime, const glz::generic& config) {
  auto dedicated_core = runtime->core_allocator().lease_core(CoreType::DEDICATED);
  dedicated_core.pin_thread("run_single_threaded_callback_app");

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

  CallbackReceiver<request::RequestType, MatchingEngine<StlL3BookContainer, ExchangeDispatcherSinkType>>
      matching_engine(config["matching_engine"], security_storage, exchange_dispatcher.get_sink());

  using MatchingEngineSinkType = std::remove_cvref_t<decltype(matching_engine.get_sink())>;

  OrderGeneratorWrapper<SimpleOrderGenerator,
      GeneratorInterface<MatchingEngineSinkType, generator_interface_skills::StoreOrders>,
      MatchingEngineSinkType>
      order_generator(runtime->stop_token(),
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

  order_generator.run();
}

}  // namespace

}  // namespace exchange_sim

using namespace exchange_sim;

int main(int argc, char* argv[]) {
  int exit_code = 0;
  CPPTRACE_TRY {
    const auto [runtime, config] = read_config_and_initialize_logging(std::span(argv, argv + argc), false);
    run_single_threaded_callback_app(runtime, config);
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
