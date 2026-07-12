#include "base/journal.h"
#include "base/run.h"

#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"

#include <boost/hof/match.hpp>
#include <cpptrace/cpptrace.hpp>
#include <cpptrace/from_current.hpp>

#include <iostream>

namespace exchange_sim {

struct RequestStat {
  Ticks::Underlying origin_time;
  enum class Type : std::uint8_t { ADD = 0, CANCEL = 1, REPLACE = 2 } type;

  std::optional<std::int64_t> first_report_ns = std::nullopt;
  std::optional<std::int64_t> last_report_ns = std::nullopt;

  std::optional<std::int64_t> first_market_event_ns = std::nullopt;
  std::optional<std::int64_t> last_market_event_ns = std::nullopt;

  size_t reports_count = 0;
  size_t market_events_count = 0;
  size_t trades_count = 0;
};

}  // namespace exchange_sim

template<>
struct glz::meta<exchange_sim::RequestStat> {
  using T = exchange_sim::RequestStat;
  static constexpr auto value = object("origin_time",
      &T::origin_time,
      "type",
      &T::type,
      "first_report",
      &T::first_report_ns,
      "last_report",
      &T::last_report_ns,
      "first_market_event",
      &T::first_market_event_ns,
      "last_market_event",
      &T::last_market_event_ns,
      "reports_count",
      &T::reports_count,
      "market_events_count",
      &T::market_events_count,
      "trades_count",
      &T::trades_count);
};

namespace exchange_sim {

void run_calc_stats(const std::shared_ptr<Runtime>&, const glz::generic& config) {
  Journal<market_events::EventType> market_data_journal(
      "Journal<market_events::EventType>", config["journaling"]["market_events"].as<std::string>(), JournalMode::READ);
  Journal<report::ReportType> report_journal(
      "Journal<report::ReportType>", config["journaling"]["reports"].as<std::string>(), JournalMode::READ);
  Journal<request::RequestType> request_journal(
      "Journal<request::RequestType>", config["journaling"]["requests"].as<std::string>(), JournalMode::READ);

  std::deque<decltype(report_journal)::value_type> current_reports;
  std::deque<decltype(market_data_journal)::value_type> current_market_events;

  auto report_iterator = report_journal.begin();
  auto market_data_iterator = market_data_journal.begin();

  static constexpr std::size_t constMaxStructSize = 100;

  std::fstream output_file(config["calc_stats"]["output_file"].as<std::string>(), std::ios::out);
  std::string write_buffer;

  output_file << "[\n";
  size_t processed_count = 0;
  auto requests_to_skip = config["calc_stats"]["requests_to_skip"].as<size_t>();
  bool first_to_write = true;

  Throughput requests_throughput("requests");
  for (const auto& request_struct : request_journal) {
    processed_count++;
    requests_throughput.tick();
    while (!current_reports.empty() && current_reports.front().timestamp < request_struct.timestamp) {
      current_reports.pop_front();
    }
    while (current_reports.size() < constMaxStructSize && report_iterator != report_journal.end()) {
      current_reports.push_back(*report_iterator);
      ++report_iterator;
    }

    while (!current_market_events.empty() && current_market_events.front().timestamp < request_struct.timestamp) {
      current_market_events.pop_front();
    }
    while (current_market_events.size() < constMaxStructSize && market_data_iterator != market_data_journal.end()) {
      current_market_events.push_back(*market_data_iterator);
      ++market_data_iterator;
    }

    if (processed_count <= requests_to_skip) {
      continue;
    }

    RequestStat stat{.origin_time = request_struct.timestamp.ticks(), .type = [&]() {
                       return std::visit(
                           boost::hof::match([](const request::OrderAddRequest&) { return RequestStat::Type::ADD; },
                               [](const request::OrderCancelRequest&) { return RequestStat::Type::CANCEL; },
                               [](const request::OrderReplaceRequest&) { return RequestStat::Type::REPLACE; }),
                           request_struct.value);
                     }()};

    for (const auto& market_event_struct : current_market_events) {
      std::visit(
          [&](auto&& current_market_event) {
            if (current_market_event.origin_time.ticks() == stat.origin_time) {
              auto time_ns =
                  market_event_struct.timestamp.nanoseconds_distance(current_market_event.origin_time).count();
              if (!stat.first_market_event_ns.has_value() || time_ns < stat.first_market_event_ns.value()) {
                stat.first_market_event_ns = time_ns;
              }
              if (!stat.last_market_event_ns.has_value() || time_ns > stat.last_market_event_ns.value()) {
                stat.last_market_event_ns = time_ns;
              }
              stat.market_events_count++;
              if constexpr (std::is_same_v<std::remove_cvref_t<decltype(current_market_event)>, market_events::Trade>) {
                stat.trades_count++;
              }
            }
          },
          market_event_struct.value);
    }

    for (const auto& report_struct : current_reports) {
      std::visit(
          [&](auto&& current_report) {
            if (current_report.origin_time.ticks() == stat.origin_time) {
              auto time_ns = report_struct.timestamp.nanoseconds_distance(current_report.origin_time).count();
              if (!stat.first_report_ns.has_value() || time_ns < stat.first_report_ns.value()) {
                stat.first_report_ns = time_ns;
              }
              if (!stat.last_report_ns.has_value() || time_ns > stat.last_report_ns.value()) {
                stat.last_report_ns = time_ns;
              }
              stat.reports_count++;
            }
          },
          report_struct.value);
    }

    write_buffer.clear();
    if (!first_to_write) {
      output_file << ",";
    } else {
      first_to_write = false;
    }
    CHECK(!glz::write_json(stat, write_buffer), "Failed to print json");
    output_file << write_buffer;
  }
  output_file << "\n]";
}

}  // namespace exchange_sim


using namespace exchange_sim;

int main(int argc, char* argv[]) {
  int exit_code = 0;
  CPPTRACE_TRY {
    const auto [runtime, config] = read_config_and_initialize_logging(std::span(argv, argv + argc));
    run_calc_stats(runtime, config);
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