#include "base/callback_sink.h"

#include "exchange/book_containers/stl_book_container.h"
#include "exchange/domain/engine_events.h"
#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/input/maintain_book_skill.h"
#include "exchange/matching/matching_engine.h"
#include "exchange/output/exchange_dispatcher.h"
#include "exchange/tests/test_input.h"

#include <gtest/gtest.h>

#include <map>
#include <utility>
#include <variant>
#include <vector>

using namespace exchange_sim;

namespace {

class MaintainBookMatchesMatchingEngineTest : public ::testing::TestWithParam<TestInput> {};

}  // namespace

TEST_P(MaintainBookMatchesMatchingEngineTest, MatchesMatchingEngine) {
  auto snapshot = [](const auto& container) {
    std::map<std::pair<Direction, Price>, std::vector<std::pair<ExchangeOrderID, Amount>>> snapshot;

    for (const auto& [price, level] : container.template half<Direction::BUY>()) {
      auto& snapshot_level = snapshot[{Direction::BUY, price}];
      for (const auto& order : level) {
        snapshot_level.emplace_back(order.order_id, order.quantity);
      }
    }
    for (const auto& [price, level] : container.template half<Direction::SELL>()) {
      auto& snapshot_level = snapshot[{Direction::SELL, price}];
      for (const auto& order : level) {
        snapshot_level.emplace_back(order.order_id, order.quantity);
      }
    }
    return snapshot;
  };

  const auto& input = GetParam();

  generator_interface_skills::MaintainBook<StlL3BookContainer> maintain_book_skill(security_storage());
  CallbackSink market_data_sink([&](market_events::EventType&& event) {
    std::visit([&](auto&& event) { maintain_book_skill.handle_market_event(event); }, event);
  });
  CallbackSink report_sink([&](report::ReportType&&) {});
  ExchangeDispatcher exchange_dispatcher(glz::generic{}, market_data_sink, report_sink);
  CallbackSink engine_events_sink([&](engine_events::EventType&& event) { exchange_dispatcher.process(event); });

  MatchingEngine<StlL3BookContainer, decltype(engine_events_sink)> matching_engine(
      glz::generic{}, security_storage(), engine_events_sink);

  for (const auto& request : input.requests) {
    matching_engine.process(request);
    for (const Security& security : security_storage().securities()) {
      EXPECT_EQ(snapshot(maintain_book_skill.book_storage()[security.security_id].container()),
          snapshot(matching_engine.books()[security.security_id].container()));
    }
  }
}

INSTANTIATE_TEST_SUITE_P(MaintainBookMatchesMatchingEngineTest,
    MaintainBookMatchesMatchingEngineTest,
    ::testing::ValuesIn(all_test_inputs()),
    [](const ::testing::TestParamInfo<TestInput>& info) { return info.param.description; });
