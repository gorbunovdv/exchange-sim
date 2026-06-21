#include "base/run.h"
#include "base/logging/logging_backend.h"

namespace exchange_sim {

namespace {
void run_empty_app(const glz::generic& config) {
  INFO("config={}", glz::write_json(config).value());
}

}
}

int main(int argc, char** argv) {
  auto config = exchange_sim::read_config_and_initialize_logging(std::span(argv, argv + argc));
  exchange_sim::run_empty_app(config);
  exchange_sim::LoggingBackend::instance().get_logger()->flush_log();
  return 0;
}