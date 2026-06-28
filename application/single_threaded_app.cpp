#include "base/run.h"
#include "base/logging/logging_backend.h"
#include "application/simple_order_generator.h"
#include "application/matching_engine.h"
#include "application/simple_receiver.h"

#include <iostream>

#include "market_data_generator.h"
#include "report_generator.h"

namespace exchange_sim {

void run_single_threaded_app(const glz::generic& config) {
  auto market_data_generator =
      SimpleReceiver<MarketDataSnapshotType, MarketDataGenerator>(config["market_data_generator"]);
  auto report_generator = SimpleReceiver<OrderReportType, ReportGenerator>(config["report_generator"]);
  auto matching_engine = SimpleReceiver<RequestType, MatchingEngine<CallbackSink, CallbackSink>>(
      config["matching_engine"], market_data_generator.get_sink(), report_generator.get_sink());
  SimpleOrderGenerator order_generator(config["order_generator"], matching_engine.get_sink());
  order_generator.run();
}

}  // namespace exchange_sim

int main(int argc, char* argv[]) {
  try {
    const auto config = exchange_sim::read_config_and_initialize_logging(std::span(argv, argv + argc));
    exchange_sim::run_single_threaded_app(config);
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred: " << e.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception occurred" << '\n';
    return 1;
  }
  return 0;
}