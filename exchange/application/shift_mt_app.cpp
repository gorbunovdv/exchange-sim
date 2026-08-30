#include "base/journal.h"
#include "base/run.h"
#include "base/thread_receiver.h"

#include "domain/generate_securities.h"
#include "domain/security_storage.h"

#include "exchange/book_containers/stl_book_container.h"
#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"
#include "exchange/input/generator_interface.h"
#include "exchange/input/maintain_book_skill.h"
#include "exchange/input/order_generator_wrapper.h"
#include "exchange/input/shift_order_generator.h"
#include "exchange/input/simple_order_generator.h"
#include "exchange/input/store_orders_skill.h"
#include "exchange/matching/matching_engine.h"
#include "exchange/output/exchange_dispatcher.h"
#include "run_multi_threaded_app.h"

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>
#include <quill/Backend.h>

#include <iostream>

namespace exchange_sim {

namespace {

template<typename GeneratorInterface>
using AppShiftOrderGenerator = ShiftOrderGenerator<StlL3BookContainer, GeneratorInterface>;

template<typename SinkType>
using AppOrderGenerator = OrderGeneratorWrapper<AppShiftOrderGenerator,
    GeneratorInterface<SinkType,
        generator_interface_skills::StoreOrders, generator_interface_skills::MaintainBook<StlL3BookContainer>>,
    SinkType>;


template<typename SinkType>
using AppMatchingEngine = MatchingEngine<StlL3BookContainer, SinkType>;

template<typename MDSinkType, typename ORSinkType>
using AppExchangeDispatcher = ExchangeDispatcher<MDSinkType, ORSinkType>;

}  // namespace

}  // namespace exchange_sim

using namespace exchange_sim;

int main(int argc, char* argv[]) {
  int exit_code = 0;
  CPPTRACE_TRY {
    const auto [runtime, config] = read_config_and_initialize_logging(std::span(argv, argv + argc), false);
    exchange_sim::run_multi_threaded_event_loop_app<AppOrderGenerator, AppMatchingEngine, AppExchangeDispatcher>(
        runtime, config);
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
}//
// Created by gorbunovdv on 8/30/26.
//
