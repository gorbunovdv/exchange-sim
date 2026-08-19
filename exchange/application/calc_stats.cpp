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

#include <algorithm>
#include <cmath>
#include <iostream>

namespace exchange_sim {

struct RequestStat {
  Ticks::Underlying origin_time;
  std::string type;
  std::string symbol;

  std::optional<std::int64_t> first_report_ns = std::nullopt;
  std::optional<std::int64_t> last_report_ns = std::nullopt;

  std::optional<std::int64_t> first_market_event_ns = std::nullopt;
  std::optional<std::int64_t> last_market_event_ns = std::nullopt;

  size_t reports_count = 0;
  size_t market_events_count = 0;
  size_t trades_count = 0;
};

struct MetricStat {
  std::string metric;
  std::string symbol;
  std::string type;
  size_t trades_count;
  size_t events_count;
  std::string percentile;
  std::int64_t value;
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

  static constexpr std::array constLatencyFields = {
      "first_report", "last_report", "first_market_event", "last_market_event"};
  std::map<std::tuple<std::string, std::string, size_t>,
      std::array<std::vector<std::int64_t>, constLatencyFields.size()>>
      latencies_by_group;

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
      return std::visit(boost::hof::match([](const request::OrderAddRequest&) { return "ADD"; },
                            [](const request::OrderCancelRequest&) { return "CANCEL"; },
                            [](const request::OrderReplaceRequest&) { return "REPLACE"; }),
          request_object);
    }();
    stat.symbol = security_storage
                      .get_security(std::visit([](const auto& request) { return request.security_id; }, request_object))
                      .symbol.value();
    write_buffer.clear();
    if (!first_to_write) {
      output_file << ",\n";
    } else {
      first_to_write = false;
    }
    CHECK(!glz::write_json(stat, write_buffer), "Failed to print json");
    output_file << write_buffer;

    auto& group_latencies = latencies_by_group[{stat.symbol, stat.type, stat.trades_count}];
    const std::array latencies = {
        stat.first_report_ns, stat.last_report_ns, stat.first_market_event_ns, stat.last_market_event_ns};
    for (size_t i = 0; i < latencies.size(); i++) {
      if (latencies[i].has_value()) {
        group_latencies[i].push_back(latencies[i].value());
      }
    }
  }
  output_file << "\n]";

  static constexpr std::array<std::pair<double, std::string_view>, 7> constPercentiles = {
      {{0, "0"}, {50, "50"}, {75, "75"}, {90, "90"}, {99, "99"}, {99.9, "99.9"}, {100, "100"}}};

  auto percentile_value = [](std::vector<std::int64_t>& values, double percentile) {
    auto index = static_cast<int32_t>(std::ceil(percentile / 100 * static_cast<double>(values.size())) - 1);
    index = std::max(0, std::min<int32_t>(std::ssize(values) - 1, index));
    std::ranges::nth_element(values, values.begin() + index);
    return values[index];
  };

  std::vector<MetricStat> stats;
  for (size_t field_index = 0; field_index < constLatencyFields.size(); field_index++) {
    for (auto& [group, group_latencies] : latencies_by_group) {
      auto& values = group_latencies[field_index];
      if (values.empty()) {
        continue;
      }
      const auto& [symbol, type, trades_count] = group;
      for (const auto& [percentile, percentile_label] : constPercentiles) {
        stats.push_back(MetricStat{
            .metric = constLatencyFields[field_index],
            .symbol = symbol,
            .type = type,
            .trades_count = trades_count,
            .events_count = values.size(),
            .percentile = std::string(percentile_label),
            .value = percentile_value(values, percentile),
        });
      }
    }
  }

  write_buffer.clear();
  CHECK(!glz::write<glz::opts{.prettify = true}>(stats, write_buffer), "Failed to print json");
  std::fstream stats_file(config["results"]["stats"].as<std::string>(), std::ios::out);
  stats_file << write_buffer << std::flush;
  CHECK(stats_file, "Failed to write stats, path={}", config["results"]["stats"].as<std::string>());
}

}  // namespace exchange_sim


using namespace exchange_sim;

int main(int argc, char* argv[]) {
  int exit_code = 0;
  CPPTRACE_TRY {
    const auto [runtime, config] = read_config_and_initialize_logging(std::span(argv, argv + argc), true);
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