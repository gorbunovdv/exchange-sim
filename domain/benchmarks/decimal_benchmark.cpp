#include "base/config/config.h"
#include "base/random.h"
#include "base/run.h"

#include "domain/generate_securities.h"
#include "domain/security_storage.h"

#include <boost/decimal.hpp>
#include <cpptrace/from_current.hpp>
#include <quill/Backend.h>

#include <benchmark/benchmark.h>

namespace exchange_sim {

namespace {

struct BenchmarkMemory {
  Random random;
  SecurityStorage security_storage;
};

std::optional<BenchmarkMemory> memory;

template<typename Decimal>
void BenchmarkDecimal(benchmark::State& state) {
  static constexpr SecurityID constSecurityId{0};

  CHECK(memory.has_value(), "Memory is not initialized");

  const Security& security = memory->security_storage.get_security(constSecurityId);

  std::map<Decimal, std::list<Amount>> levels;
  for (int64_t i = 0; i < state.range(0); i++) {
    auto price = memory->random.rand_decimal<Decimal>(
        Decimal{security.low_price.value()}, Decimal{security.high_price.value()}, Decimal{security.tick_size.value()});
    auto amount = memory->random.rand_decimal<Amount>(Amount{0}, security.max_amount, security.lot_size);
    levels[price].push_back(amount);
  }

  static constexpr size_t constMaxQueryCount = 1u << 20u;

  std::vector<Decimal> queries;
  queries.reserve(constMaxQueryCount);
  for (size_t i = 0; i < constMaxQueryCount; i++) {
    queries.push_back(memory->random.rand_decimal<Decimal>(Decimal{security.low_price.value()},
        Decimal{security.high_price.value()},
        Decimal{security.tick_size.value()}));
  }

  size_t current_query = 0;

  for (auto _ : state) {
    benchmark::DoNotOptimize(levels.find(queries[current_query++ % constMaxQueryCount]));
  }
}

void run_decimal_benchmark(const std::shared_ptr<Runtime>& runtime, const glz::generic& json_config) {
  CoreLease core = runtime->core_allocator().lease_core(CoreType::DEDICATED);
  core.pin_thread("decimal_benchmark");
  memory = {
      .random = Random{json_config["order_generator"]["seed"].as<std::uint32_t>()},
      .security_storage = SecurityStorage{generate_securities(json_config)},
  };
  benchmark::RunSpecifiedBenchmarks();
}

}  // namespace

}  // namespace exchange_sim

using namespace exchange_sim;

BENCHMARK_TEMPLATE(BenchmarkDecimal, boost::decimal::decimal32_t)->Range(2, 1024);
BENCHMARK_TEMPLATE(BenchmarkDecimal, boost::decimal::decimal64_t)->Range(2, 1024);
BENCHMARK_TEMPLATE(BenchmarkDecimal, boost::decimal::decimal_fast32_t)->Range(2, 1024);
BENCHMARK_TEMPLATE(BenchmarkDecimal, boost::decimal::decimal_fast64_t)->Range(2, 1024);
BENCHMARK_TEMPLATE(BenchmarkDecimal, int32_t)->Range(2, 1024);
BENCHMARK_TEMPLATE(BenchmarkDecimal, int64_t)->Range(2, 1024);

int main(int argc, char* argv[]) {
  int exit_code = 0;
  CPPTRACE_TRY {
    benchmark::Initialize(&argc, argv);
    const auto [runtime, config] = read_config_and_initialize_logging(std::span(argv, argv + argc), false);
    run_decimal_benchmark(runtime, config);
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