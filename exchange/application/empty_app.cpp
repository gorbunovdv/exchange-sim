#include "base/logging/logging_backend.h"
#include "base/logging/quill_sinks.h"
#include "base/run.h"

#include <glaze/glaze.hpp>
#include <quill/Backend.h>

#include <iostream>
#include <span>
#include <stdexcept>

namespace exchange_sim {

namespace {

void run_empty_app(const std::shared_ptr<Runtime>&, const glz::generic& config) {
  INFO("config={}", glz::write_json(config).value());
}

}  // namespace
}  // namespace exchange_sim

using namespace exchange_sim;

int main(int argc, char** argv) {
  try {
    auto [runtime, config] = read_config_and_initialize_logging(std::span(argv, argv + argc));
    run_empty_app(runtime, config);
    quill::Backend::stop();
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred: " << e.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception occurred" << '\n';
    return 1;
  }
  return 0;
}