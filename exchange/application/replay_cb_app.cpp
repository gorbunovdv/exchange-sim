#include "base/journal.h"
#include "base/run.h"

#include "domain/generate_securities.h"

#include "exchange/application/run_single_threaded_callback_app.h"
#include "exchange/book_containers/stl_book_container.h"
#include "exchange/input/generator_interface.h"
#include "exchange/input/pending_transactions_skill.h"
#include "exchange/input/replay_order_generator.h"
#include "exchange/matching/matching_engine.h"
#include "exchange/output/exchange_dispatcher.h"

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>
#include <quill/Backend.h>

#include <iostream>

using namespace exchange_sim;

namespace {

template<typename SinkType>
using AppOrderGenerator = OrderGeneratorWrapper<ReplayOrderGenerator,
    GeneratorInterface<SinkType, generator_interface_skills::PendingTransactions>,
    SinkType>;

template<typename SinkType>
using AppMatchingEngine = MatchingEngine<StlL3BookContainer, SinkType>;

template<typename MDSinkType, typename ORSinkType>
using AppExchangeDispatcher = ExchangeDispatcher<MDSinkType, ORSinkType>;

}  // namespace

int main(int argc, char* argv[]) {
  int exit_code = 0;
  CPPTRACE_TRY {
    const auto [runtime, config] = read_config_and_initialize_logging(std::span(argv, argv + argc), false);
    run_single_threaded_callback_app<AppOrderGenerator, AppMatchingEngine, AppExchangeDispatcher>(runtime, config);
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
