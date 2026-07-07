#include "base/callback_receiver.h"
#include "base/forward_sink.h"
#include "base/run.h"

#include "domain/security.h"

#include "exchange/domain/engine_events.h"
#include "exchange/input/order_generator_wrapper.h"
#include "exchange/input/simple_order_generator.h"
#include "exchange/matching/matching_engine.h"
#include "exchange/output/exchange_dispatcher.h"

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>

#include <iostream>

namespace exchange_sim {

namespace {

std::vector<SecurityConfig> generate_securities(const glz::generic& config) {
  std::vector<SecurityConfig> securities;
  securities.reserve(config["securities"].size());
  for (const auto& security_config : config["securities"].get_array()) {
    securities.emplace_back(SecurityConfig{
        .symbol = Symbol(security_config["symbol"].as<std::string>()),
        .tick_size = Price(security_config["tick_size"].as<double>()),
        .lot_size = Amount(security_config["lot_size"].as<double>()),
        .low_price = Price(security_config["low_price"].as<double>()),
        .high_price = Price(security_config["high_price"].as<double>()),
        .max_amount = Amount(security_config["max_amount"].as<double>()),
    });
  }
  return securities;
}

void run_single_threaded_callback_app(const glz::generic& config) {
  SecurityStorage security_storage(generate_securities(config));
  for (Security security : security_storage.securities()) {
    INFO("Added security, symbol={} lot_size={} tick_size={} low_price={} high_price={} max_amount={}",
        std::string(security.symbol),
        static_cast<double>(security.lot_size),
        static_cast<double>(security.tick_size),
        static_cast<double>(security.low_price),
        static_cast<double>(security.high_price),
        static_cast<double>(security.max_amount));
  }

  using MarketDataSinkType = ForwardSink<market_events::EventType, CallbackSink<market_events::EventType>>;
  using ReportSinkType = ForwardSink<report::ReportType, CallbackSink<report::ReportType>>;

  MarketDataSinkType market_data_generator_sink;
  ReportSinkType report_generator_sink;

  CallbackReceiver<engine_events::EventType, ExchangeDispatcher<MarketDataSinkType, ReportSinkType>>
      exchange_dispatcher(config["exchange_dispatcher"], market_data_generator_sink, report_generator_sink);

  using ExchangeDispatcherSinkType = std::remove_cvref_t<decltype(exchange_dispatcher.get_sink())>;

  CallbackReceiver<request::RequestType, MatchingEngine<ExchangeDispatcherSinkType>> matching_engine(
      config["matching_engine"], security_storage, exchange_dispatcher.get_sink());

  using MatchingEngineSinkType = std::remove_cvref_t<decltype(matching_engine.get_sink())>;

  OrderGeneratorWrapper<SimpleOrderGenerator, GeneratorInterface, MatchingEngineSinkType> order_generator(
      security_storage, matching_engine.get_sink(), config["order_generator"]);

  CallbackSink<market_events::EventType> market_data_generator_sink_callback(
      [&order_generator](market_events::EventType&& snapshot) { order_generator.process(std::move(snapshot)); });
  CallbackSink<report::ReportType> report_generator_sink_callback(
      [&order_generator](report::ReportType&& report) { order_generator.process(std::move(report)); });

  market_data_generator_sink.set_sink(&market_data_generator_sink_callback);
  report_generator_sink.set_sink(&report_generator_sink_callback);

  order_generator.run();
}

}  // namespace

}  // namespace exchange_sim

int main(int argc, char* argv[]) {
  int exit_code = 0;
  CPPTRACE_TRY {
    const auto config = exchange_sim::read_config_and_initialize_logging(std::span(argv, argv + argc));
    exchange_sim::run_single_threaded_callback_app(config);
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