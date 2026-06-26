#include <iostream>

#include "base/logging/logging_backend.h"
#include "base/run.h"

namespace exchange_sim {

namespace {

void run_empty_app(const glz::generic& config) {
  INFO("config={}", glz::write_json(config).value());
}

}  // namespace
}  // namespace exchange_sim

int main(int argc, char** argv) {
  try {
    auto config = exchange_sim::read_config_and_initialize_logging(std::span(argv, argv + argc));
    exchange_sim::run_empty_app(config);
    exchange_sim::LoggingBackend::instance().get_logger()->flush_log();
  } catch (const std::exception& e) {
    std::cerr << "Exception occured: " << e.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception occured" << '\n';
    return 1;
  }
  return 0;
}