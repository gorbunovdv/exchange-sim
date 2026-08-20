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

namespace {

class RequestMatcher {
public:
  explicit RequestMatcher(const SecurityStorage& security_storage,
      const std::filesystem::path& market_events_path,
      const std::filesystem::path& reports_path,
      const std::filesystem::path& requests_path):
      market_data_journal_("Journal<market_events::EventType>", market_events_path, JournalMode::READ),
      report_journal_("Journal<report::ReportType>", reports_path, JournalMode::READ),
      request_journal_("Journal<request::RequestType>", requests_path, JournalMode::READ),
      security_storage_(security_storage),
      market_data_iterator_(market_data_journal_.begin()),
      report_iterator_(report_journal_.begin()),
      request_iterator_(request_journal_.begin()) {}

  std::optional<RequestStat> next() {
    static constexpr std::chrono::nanoseconds constMaxTimeDifference = 1ms;

    if (request_iterator_ == request_journal_.end()) {
      return std::nullopt;
    }

    const auto& [request_timestamp, request_object] = *request_iterator_;

    while (!current_reports_.empty() && current_reports_.front().timestamp < request_timestamp) {
      current_reports_.pop_front();
    }
    while (report_iterator_ != report_journal_.end() &&
        (current_reports_.empty() ||
            current_reports_.back().timestamp.nanoseconds_distance(request_timestamp) < constMaxTimeDifference)) {
      current_reports_.push_back(*report_iterator_);
      ++report_iterator_;
      std::visit(
          [&](auto&& report) {
            update_report(request_by_origin_time_[report.origin_time], current_reports_.back().timestamp, report);
            CHECK(report.origin_time >= request_timestamp,
                "constMaxTimeDifference is too small, some reports are being missed");
          },
          current_reports_.back().value);
    }

    while (!current_market_events.empty() && current_market_events.front().timestamp < request_timestamp) {
      current_market_events.pop_front();
    }
    while (market_data_iterator_ != market_data_journal_.end() &&
        (current_market_events.empty() ||
            current_market_events.back().timestamp.nanoseconds_distance(request_timestamp) < constMaxTimeDifference)) {
      current_market_events.push_back(*market_data_iterator_);
      ++market_data_iterator_;
      std::visit(
          [&](auto&& market_event) {
            update_market_data(request_by_origin_time_[market_event.origin_time],
                current_market_events.back().timestamp,
                market_event);
            CHECK(market_event.origin_time >= request_timestamp,
                "constMaxTimeDifference is too small, some market events are being missed");
          },
          current_market_events.back().value);
    }

    while (!request_by_origin_time_.empty() && request_by_origin_time_.begin()->first < request_timestamp) {
      request_by_origin_time_.erase(request_by_origin_time_.begin());
    }

    RequestStat& stat = request_by_origin_time_.at(request_timestamp);
    stat.origin_time = request_timestamp.ticks();
    stat.type = [](const auto& request_object) {
      return std::visit(boost::hof::match([](const request::OrderAddRequest&) { return "ADD"; },
                            [](const request::OrderCancelRequest&) { return "CANCEL"; },
                            [](const request::OrderReplaceRequest&) { return "REPLACE"; }),
          request_object);
    }(request_object);
    SecurityID security_id = std::visit([](const auto& request) { return request.security_id; }, request_object);
    stat.symbol = security_storage_.get_security(security_id).symbol.value();
    request_iterator_++;
    return stat;
  }

private:
  static void update_report(RequestStat& stat, Ticks report_timestamp, const auto& report) {
    auto time_ns = report_timestamp.nanoseconds_distance(report.origin_time).count();
    if (!stat.first_report_ns.has_value() || time_ns < stat.first_report_ns.value()) {
      stat.first_report_ns = time_ns;
    }
    if (!stat.last_report_ns.has_value() || time_ns > stat.last_report_ns.value()) {
      stat.last_report_ns = time_ns;
    }
    stat.reports_count++;
  };

  static void update_market_data(RequestStat& stat, Ticks market_event_timestamp, const auto& market_event) {
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

  Journal<market_events::EventType> market_data_journal_;
  Journal<report::ReportType> report_journal_;
  Journal<request::RequestType> request_journal_;
  const SecurityStorage& security_storage_;

  std::map<Ticks, RequestStat> request_by_origin_time_;

  std::deque<decltype(report_journal_)::value_type> current_reports_;
  std::deque<decltype(market_data_journal_)::value_type> current_market_events;

  decltype(market_data_journal_)::iterator<> market_data_iterator_;
  decltype(report_journal_)::iterator<> report_iterator_;
  decltype(request_journal_)::iterator<> request_iterator_;
};

constexpr std::array constLatencyFields = {
    "first_report",
    "last_report",
    "first_market_event",
    "last_market_event",
};

constexpr std::array<std::pair<double, std::string_view>, 7> constPercentiles = {
    {
        {0, "0"},
        {50, "50"},
        {75, "75"},
        {90, "90"},
        {99, "99"},
        {99.9, "99.9"},
        {100, "100"},
    },
};

void run_calc_stats(const std::shared_ptr<Runtime>& runtime, const glz::generic& config) {
  auto stop_token = runtime->stop_token();

  SecurityStorage security_storage(generate_securities(config));

  RequestMatcher request_matcher(security_storage,
      config["journaling"]["market_events"].as<std::string>(),
      config["journaling"]["reports"].as<std::string>(),
      config["journaling"]["requests"].as<std::string>());

  std::map<std::tuple<std::string, std::string, size_t>,
      std::array<std::vector<std::int64_t>, constLatencyFields.size()>>
      latencies_by_group;

  std::fstream output_file(config["calc_stats"]["output_file"].as<std::string>(), std::ios::out);
  output_file << "[\n";

  size_t processed_count = 0;
  auto requests_to_skip = config["calc_stats"]["requests_to_skip"].as<size_t>();
  bool first_to_write = true;

  auto update_by_stat = [&](const auto& stat) {
    auto& group_latencies = latencies_by_group[{stat.symbol, stat.type, stat.trades_count}];
    const std::array latencies = {
        stat.first_report_ns,
        stat.last_report_ns,
        stat.first_market_event_ns,
        stat.last_market_event_ns,
    };
    for (size_t i = 0; i < latencies.size(); i++) {
      if (latencies[i].has_value()) {
        group_latencies[i].push_back(latencies[i].value());
      }
    }
  };

  std::string write_buffer;

  auto write_stat = [&](auto stat) {
    write_buffer.clear();
    if (!first_to_write) {
      output_file << ",\n";
    } else {
      first_to_write = false;
    }
    CHECK(!glz::write_json(stat, write_buffer), "Failed to print json");
    output_file << write_buffer;
    CHECK(output_file, "Failed to write output_file, path={}", config["calc_stats"]["output_file"].as<std::string>());
  };

  Throughput throughput("calc_stats.cpp");

  while (auto opt_st = request_matcher.next()) {
    if (stop_token.stop_requested()) {
      break;
    }
    const auto& stat = *opt_st;
    processed_count++;
    if (processed_count <= requests_to_skip) {
      continue;
    }
    write_stat(stat);
    update_by_stat(stat);
    throughput.tick();
  }
  output_file << "\n]";

  auto percentile_value = [](std::vector<std::int64_t>& values, double percentile) {
    auto index = static_cast<int32_t>(std::ceil(percentile / 100 * static_cast<double>(values.size())) - 1);
    index = std::max(0, std::min(static_cast<int32_t>(values.size()) - 1, index));
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
  stats_file << write_buffer;
  CHECK(stats_file, "Failed to write stats, path={}", config["results"]["stats"].as<std::string>());
}


}  // namespace

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