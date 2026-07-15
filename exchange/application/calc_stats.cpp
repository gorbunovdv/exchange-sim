#include "base/journal.h"
#include "base/run.h"

#include "domain/security_storage.h"

#include "exchange/application/helpers/generate_securities.h"
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
  std::string symbol;

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
      "symbol",
      &T::symbol,
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

void run_calc_stats(const std::shared_ptr<Runtime>& runtime, const glz::generic& config) {
  auto stop_token = runtime->stop_token();

  SecurityStorage security_storage(generate_securities(config));

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

  static constexpr std::chrono::nanoseconds constMaxTimeDifference = 1ms;

  std::fstream output_file(config["calc_stats"]["output_file"].as<std::string>(), std::ios::out);
  std::string write_buffer;

  output_file << "[\n";
  size_t processed_count = 0;
  auto requests_to_skip = config["calc_stats"]["requests_to_skip"].as<size_t>();
  bool first_to_write = true;

  std::map<Ticks, RequestStat> request_by_origin_time;

  auto update_report = [](RequestStat& stat, Ticks report_timestamp, const auto& report) {
    auto time_ns = report_timestamp.nanoseconds_distance(report.origin_time).count();
    if (!stat.first_report_ns.has_value() || time_ns < stat.first_report_ns.value()) {
      stat.first_report_ns = time_ns;
    }
    if (!stat.last_report_ns.has_value() || time_ns > stat.last_report_ns.value()) {
      stat.last_report_ns = time_ns;
    }
    stat.reports_count++;
  };

  auto update_market_data = [](RequestStat& stat, Ticks market_event_timestamp, const auto& market_event) {
    auto time_ns = market_event_timestamp.nanoseconds_distance(market_event.origin_time).count();
    if (!stat.first_market_event_ns.has_value() || time_ns < stat.first_market_event_ns.value()) {
      stat.first_market_event_ns = time_ns;
    }
    if (!stat.last_market_event_ns.has_value() || time_ns > stat.last_market_event_ns.value()) {
      stat.last_market_event_ns = time_ns;
    }
    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(market_event)>, market_events::Trade>) {
      stat.trades_count++;
    }
    stat.market_events_count++;
  };

  Throughput requests_throughput("requests");
  for (const auto& [request_timestamp, request_object] : request_journal) {
    if (stop_token.stop_requested()) {
      break;
    }
    processed_count++;
    requests_throughput.tick();
    while (!current_reports.empty() && current_reports.front().timestamp < request_timestamp) {
      current_reports.pop_front();
    }
    while (report_iterator != report_journal.end() &&
        (current_reports.empty() ||
            current_reports.back().timestamp.nanoseconds_distance(request_timestamp) < constMaxTimeDifference)) {
      current_reports.push_back(*report_iterator);
      ++report_iterator;
      std::visit(
          [&](auto&& report) {
            update_report(request_by_origin_time[report.origin_time], current_reports.back().timestamp, report);
            CHECK(report.origin_time >= request_timestamp,
                "constMaxTimeDifference is too small, some reports are being missed");
          },
          current_reports.back().value);
    }

    while (!current_market_events.empty() && current_market_events.front().timestamp < request_timestamp) {
      current_market_events.pop_front();
    }
    while (market_data_iterator != market_data_journal.end() &&
        (current_market_events.empty() ||
            current_market_events.back().timestamp.nanoseconds_distance(request_timestamp) < constMaxTimeDifference)) {
      current_market_events.push_back(*market_data_iterator);
      ++market_data_iterator;
      std::visit(
          [&](auto&& market_event) {
            update_market_data(
                request_by_origin_time[market_event.origin_time], current_market_events.back().timestamp, market_event);
            CHECK(market_event.origin_time >= request_timestamp,
                "constMaxTimeDifference is too small, some market events are being missed");
          },
          current_market_events.back().value);
    }

    while (!request_by_origin_time.empty() && request_by_origin_time.begin()->first < request_timestamp) {
      request_by_origin_time.erase(request_by_origin_time.begin());
    }

    if (processed_count <= requests_to_skip) {
      continue;
    }

    RequestStat& stat = request_by_origin_time.at(request_timestamp);
    stat.origin_time = request_timestamp.ticks();
    stat.type = [&]() {
      return std::visit(boost::hof::match([](const request::OrderAddRequest&) { return RequestStat::Type::ADD; },
                            [](const request::OrderCancelRequest&) { return RequestStat::Type::CANCEL; },
                            [](const request::OrderReplaceRequest&) { return RequestStat::Type::REPLACE; }),
          request_object);
    }();
    stat.symbol = security_storage
                      .get_security(std::visit([](const auto& request) { return request.security_id; }, request_object))
                      .symbol;
    write_buffer.clear();
    if (!first_to_write) {
      output_file << ",\n";
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