#include "base/callback_receiver.h"
#include "base/callback_sink.h"

#include "exchange/book_containers/stl_book_container.h"
#include "exchange/matching/matching_engine.h"
#include "exchange/output/exchange_dispatcher.h"
#include "exchange/tests/test_input.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace exchange_sim;

struct MatchingOutputTestInput {
  TestInput input;
};

std::ostream& operator<<(std::ostream& os, const MatchingOutputTestInput& test) {
  return os << test.input.description;
}

class MatchingOutputTest : public ::testing::TestWithParam<MatchingOutputTestInput> {};

TEST_P(MatchingOutputTest, ReturnExpectedEvents) {
  auto compare_event = []<typename VariantType>(const VariantType& actual, const VariantType& expected) {
    std::visit(
        [&expected](const auto& actual) {
          ASSERT_TRUE(std::holds_alternative<std::remove_cvref_t<decltype(actual)>>(expected));
          EXPECT_EQ(actual, std::get<std::remove_cvref_t<decltype(actual)>>(expected));
        },
        actual);
  };

  auto test = GetParam();

  std::vector<market_events::EventType> actual_market_data;
  std::vector<report::ReportType> actual_reports;

  CallbackSink market_data_sink(
      [&actual_market_data](market_events::EventType&& event) { actual_market_data.push_back(event); });
  CallbackSink report_generator_sink(
      [&actual_reports](report::ReportType&& event) { actual_reports.push_back(event); });

  CallbackReceiver<engine_events::EventType,
      ExchangeDispatcher<decltype(market_data_sink), decltype(report_generator_sink)>>
      exchange_dispatcher(glz::generic{}, market_data_sink, report_generator_sink);
  MatchingEngine<StlL3BookContainer, std::remove_cvref_t<decltype(exchange_dispatcher.get_sink())>> matching_engine(
      glz::generic{}, storage(), exchange_dispatcher.get_sink());

  for (auto request : test.input.requests) {
    matching_engine.process(request);
  }

  ASSERT_EQ(test.input.expected_market_data.size(), actual_market_data.size());
  for (size_t i = 0; i < test.input.expected_market_data.size(); ++i) {
    compare_event(actual_market_data[i], test.input.expected_market_data[i]);
  }

  ASSERT_EQ(test.input.expected_reports.size(), actual_reports.size());
  for (size_t i = 0; i < test.input.expected_reports.size(); ++i) {
    compare_event(actual_reports[i], test.input.expected_reports[i]);
  }
}

INSTANTIATE_TEST_SUITE_P(MatchingOutputTest,
    MatchingOutputTest,
    ::testing::Values(MatchingOutputTestInput{AddLimit_Cancel},
        MatchingOutputTestInput{AddIoc_Expire},
        MatchingOutputTestInput{Cancel_Reject},
        MatchingOutputTestInput{Replace_Reject},
        MatchingOutputTestInput{AddLimitTwoClientsSameClientOrderId_CancelSecond_CancelFirst},
        MatchingOutputTestInput{AddLimit_Replace_Cancel},
        MatchingOutputTestInput{ReplaceLosesPriority_IocTradesOrder2ThenReplacementThenExpires},
        MatchingOutputTestInput{IocFullyTradesBestPricePartiallyTradesNextPrice_CancelRemaining},
        MatchingOutputTestInput{RejectedReplaceKeepsPriority_IocTradesOrder1ThenOrder2ThenExpires},
        MatchingOutputTestInput{ReplaceTradesOrderAndRests_Cancel},
        MatchingOutputTestInput{ReplaceTradesOrderAndDoesNotRest},
        MatchingOutputTestInput{IocTradesTwoLevelsThenExpires}),
    [](const ::testing::TestParamInfo<MatchingOutputTestInput>& info) { return info.param.input.description; });
