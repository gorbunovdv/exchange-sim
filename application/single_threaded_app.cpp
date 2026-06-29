#include "base/run.h"
#include "application/simple_order_generator.h"
#include "application/matching_engine.h"
#include "application/simple_receiver.h"
#include "application/exchange_snapshots.h"
#include "application/exchange_dispatcher.h"
#include "application/market_data_generator.h"
#include "application/report_generator.h"
#include "application/security.h"

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>
#include <iostream>

namespace exchange_sim {

std::vector<SecurityConfig> generate_securities(const glz::generic& config) {
  std::vector<SecurityConfig> securities;
  securities.reserve(config["securities"].size());
  for (const auto& security_config : config["securities"].get_array()) {
    securities.emplace_back(SecurityConfig{.symbol = Symbol(security_config["symbol"].as<std::string>()),
        .tick_size = Price(security_config["tick_size"].as<double>()),
        .lot_size = Amount(security_config["lot_size"].as<double>()),
        .low_price = Price(security_config["low_price"].as<double>()),
        .high_price = Price(security_config["high_price"].as<double>()),
        .max_amount = security_config["max_amount"].as<uint32_t>()});
  }
  return securities;
}

void run_single_threaded_app(const glz::generic& config) {
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
  SimpleReceiver<market_data::SnapshotType, MarketDataGenerator> market_data_generator(config["market_data_generator"]);
  SimpleReceiver<report::ReportType, ReportGenerator> report_generator(config["report_generator"]);
  SimpleReceiver<exchange_snapshot::SnapshotType, ExchangeDispatcher<CallbackSink, CallbackSink>> exchange_dispatcher(
      config["exchange_dispatcher"], market_data_generator.get_sink(), report_generator.get_sink());
  SimpleReceiver<request::RequestType, MatchingEngine<CallbackSink>> matching_engine(
      config["matching_engine"], security_storage, exchange_dispatcher.get_sink());
  SimpleOrderGenerator order_generator(security_storage, config["order_generator"], matching_engine.get_sink());
  order_generator.run();
}

}  // namespace exchange_sim

int main(int argc, char* argv[]) {
  int exit_code = 0;
  CPPTRACE_TRY {
    const auto config = exchange_sim::read_config_and_initialize_logging(std::span(argv, argv + argc));
    exchange_sim::run_single_threaded_app(config);
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