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

using namespace exchange_sim;

namespace {

struct MaintainBookMatchesMatchingEngineTestInput {
  TestInput input;
};

std::ostream& operator<<(std::ostream& os, const MaintainBookMatchesMatchingEngineTestInput& test) {
  return os << test.input.description;
}

class MaintainBookMatchesMatchingEngineTest
    : public ::testing::TestWithParam<MaintainBookMatchesMatchingEngineTestInput> {};

}  // namespace

TEST_P(MaintainBookMatchesMatchingEngineTest, MatchesMatchingEngine) {
  auto snapshot = [](const auto& container) {
    std::map<Price, std::vector<std::tuple<Direction, ExchangeOrderID, Amount>>> snapshot;

    for (const auto& [price, level] : container.template half<Direction::BUY>()) {
      for (const auto& order : level) {
        snapshot[price].emplace_back(Direction::BUY, order.order_id, order.quantity);
      }
    }
    for (const auto& [price, level] : container.template half<Direction::SELL>()) {
      for (const auto& order : level) {
        snapshot[price].emplace_back(Direction::SELL, order.order_id, order.quantity);
      }
    }
    return snapshot;
  };

  const auto& test = GetParam();

  generator_interface_skills::MaintainBook<StlL3BookContainer> maintain_book_skill(security_storage());
  CallbackSink market_data_sink([&](market_events::EventType&& event) {
    std::visit([&](auto&& event) { maintain_book_skill.handle_market_event(event); }, event);
  });
  CallbackSink report_sink([&](report::ReportType&&) {});
  ExchangeDispatcher exchange_dispatcher(glz::generic{}, market_data_sink, report_sink);
  CallbackSink engine_events_sink([&](engine_events::EventType&& event) { exchange_dispatcher.process(event); });

  MatchingEngine<StlL3BookContainer, decltype(engine_events_sink)> matching_engine(
      glz::generic{}, security_storage(), engine_events_sink);

  for (const auto& request : test.input.requests) {
    matching_engine.process(request);
    for (const Security& security : security_storage().securities()) {
      EXPECT_EQ(snapshot(maintain_book_skill.book_storage()[security.security_id].container()),
          snapshot(matching_engine.books()[security.security_id].container()));
    }
  }
}

INSTANTIATE_TEST_SUITE_P(MaintainBookMatchesMatchingEngineTest,
    MaintainBookMatchesMatchingEngineTest,
    ::testing::Values(MaintainBookMatchesMatchingEngineTestInput{AddLimit_Cancel()},
        MaintainBookMatchesMatchingEngineTestInput{AddIoc_Expire()},
        MaintainBookMatchesMatchingEngineTestInput{Cancel_Reject()},
        MaintainBookMatchesMatchingEngineTestInput{Replace_Reject()},
        MaintainBookMatchesMatchingEngineTestInput{AddLimitTwoClientsSameClientOrderId_CancelSecond_CancelFirst()},
        MaintainBookMatchesMatchingEngineTestInput{AddLimit_Replace_Cancel()},
        MaintainBookMatchesMatchingEngineTestInput{ReplaceLosesPriority_IocTradesOrder2ThenReplacement_Expire()},
        MaintainBookMatchesMatchingEngineTestInput{IocFullyTradesBestPricePartiallyTradesNextPrice_CancelRemaining()},
        MaintainBookMatchesMatchingEngineTestInput{RejectedReplaceKeepsPriority_IocTradesOrder1ThenOrder2_Expire()},
        MaintainBookMatchesMatchingEngineTestInput{ReplaceTradesOrderAndRests_Cancel()},
        MaintainBookMatchesMatchingEngineTestInput{ReplaceTradesOrderAndDoesNotRest()},
        MaintainBookMatchesMatchingEngineTestInput{IocTradesTwoLevels_Expire()}),
    [](const ::testing::TestParamInfo<MaintainBookMatchesMatchingEngineTestInput>& info) {
      return info.param.input.description;
    });
