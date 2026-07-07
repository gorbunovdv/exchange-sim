#include "base/callback_sink.h"
#include "base/simple_receiver.h"

#include "exchange/matching/matching_engine.h"
#include "exchange/output/exchange_dispatcher.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace exchange_sim;

struct MatchingOutputTestInput {
  std::vector<request::RequestType> requests;
  std::vector<market_events::EventType> expected_market_data;
  std::vector<report::ReportType> expected_reports;

  std::string test_name;
};

std::ostream& operator<<(std::ostream& os, const MatchingOutputTestInput& input) {
  return os << input.test_name;
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

  auto input = GetParam();

  std::vector<market_events::EventType> actual_market_data;
  std::vector<report::ReportType> actual_reports;

  CallbackSink<market_events::EventType> market_data_sink(
      [&actual_market_data](market_events::EventType&& event) { actual_market_data.push_back(std::move(event)); });
  CallbackSink<report::ReportType> report_generator_sink(
      [&actual_reports](report::ReportType&& event) { actual_reports.push_back(std::move(event)); });

  SecurityStorage storage({
      SecurityConfig{
          .symbol = "A",
          .tick_size = Price(1),
          .lot_size = Amount(1),
          .low_price = Price(0),
          .high_price = Price(100),
          .max_amount = Amount(1000),
      },
      SecurityConfig{
          .symbol = "B",
          .tick_size = Price(1),
          .lot_size = Amount(1),
          .low_price = Price(0),
          .high_price = Price(100),
          .max_amount = Amount(1000),
      },
  });

  SimpleReceiver<engine_events::EventType,
      ExchangeDispatcher<CallbackSink<market_events::EventType>, CallbackSink<report::ReportType>>>
      exchange_dispatcher(glz::generic{}, market_data_sink, report_generator_sink);
  MatchingEngine matching_engine(glz::generic{}, storage, exchange_dispatcher.get_sink());

  for (auto request : input.requests) {
    matching_engine.process(std::move(request));
  }

  ASSERT_EQ(input.expected_market_data.size(), actual_market_data.size());
  for (size_t i = 0; i < input.expected_market_data.size(); ++i) {
    compare_event(actual_market_data[i], input.expected_market_data[i]);
  }

  ASSERT_EQ(input.expected_reports.size(), actual_reports.size());
  for (size_t i = 0; i < input.expected_reports.size(); ++i) {
    compare_event(actual_reports[i], input.expected_reports[i]);
  }
}

const MatchingOutputTestInput AddLimit_Cancel = {
    .requests =
        {
            request::OrderAddRequest{
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                OrderType::LIMIT,
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            request::OrderCancelRequest{
                ClientID{0},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
            },
        },
    .expected_market_data =
        {
            market_events::BookOrderEntry{
                ExchangeOrderID{1},
                SecurityID{0},
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            market_events::BookOrderDeletion{
                ExchangeOrderID{1},
                SecurityID{0},
            },
        },
    .expected_reports =
        {
            report::OrderAccepted{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            report::OrderBookEntry{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            report::OrderCancelled{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
            },
        },
    .test_name = "AddLimit_Cancel",
};

const MatchingOutputTestInput AddIoc_Expire = {
    .requests =
        {
            request::OrderAddRequest{
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::IOC,
                OrderType::LIMIT,
                Direction::BUY,
                Price{1},
                Amount{1},
            },
        },
    .expected_market_data = {},
    .expected_reports =
        {
            report::OrderAccepted{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::IOC,
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            report::OrderExpired{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                engine_events::ExpirationReason::IOC_REMAINDER_EXPIRED,
            },
        },
    .test_name = "AddIoc_Expire",
};

const MatchingOutputTestInput Cancel_Reject = {
    .requests =
        {
            request::OrderCancelRequest{
                ClientID{0},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
            },
        },
    .expected_market_data = {},
    .expected_reports =
        {
            report::CancellationRejected{
                ClientID{0},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
                report::CancellationRejectedReason::ORDER_NOT_FOUND,
            },
        },
    .test_name = "Cancel_Reject",
};

const MatchingOutputTestInput Replace_Reject = {
    .requests =
        {
            request::OrderReplaceRequest{
                ClientID{0},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
                Direction::BUY,
                Price{1},
                Amount{1},
            },
        },
    .expected_market_data = {},
    .expected_reports =
        {
            report::ReplacementRejected{
                ClientID{0},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
                report::ReplacementRejectedReason::ORDER_NOT_FOUND,
            },
        },
    .test_name = "Replace_Reject",
};

const MatchingOutputTestInput AddLimitTwoClientsSameClientOrderId_CancelSecond_CancelFirst = {
    .requests =
        {
            request::OrderAddRequest{
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                OrderType::LIMIT,
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            request::OrderAddRequest{
                ClientID{2},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                OrderType::LIMIT,
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            request::OrderCancelRequest{
                ClientID{2},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
            },
            request::OrderCancelRequest{
                ClientID{1},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
            },
        },
    .expected_market_data =
        {
            market_events::BookOrderEntry{
                ExchangeOrderID{1},
                SecurityID{0},
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            market_events::BookOrderEntry{
                ExchangeOrderID{2},
                SecurityID{0},
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            market_events::BookOrderDeletion{
                ExchangeOrderID{2},
                SecurityID{0},
            },
            market_events::BookOrderDeletion{
                ExchangeOrderID{1},
                SecurityID{0},
            },
        },
    .expected_reports =
        {
            report::OrderAccepted{
                ExchangeOrderID{1},
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            report::OrderBookEntry{
                ExchangeOrderID{1},
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            report::OrderAccepted{
                ExchangeOrderID{2},
                ClientID{2},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            report::OrderBookEntry{
                ExchangeOrderID{2},
                ClientID{2},
                ClientOrderID{0},
                SecurityID{0},
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            report::OrderCancelled{
                ExchangeOrderID{2},
                ClientID{2},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
            },
            report::OrderCancelled{
                ExchangeOrderID{1},
                ClientID{1},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
            },
        },
    .test_name = "AddLimitTwoClientsSameClientOrderId_CancelSecond_CancelFirst",
};

const MatchingOutputTestInput AddLimit_Replace_Cancel = {
    .requests =
        {
            request::OrderAddRequest{
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                OrderType::LIMIT,
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            request::OrderReplaceRequest{
                ClientID{0},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
                Direction::BUY,
                Price{2},
                Amount{1},
            },
            request::OrderCancelRequest{
                ClientID{0},
                ClientOrderID{1},
                ClientOrderID{2},
                SecurityID{0},
            },
        },
    .expected_market_data =
        {
            market_events::BookOrderEntry{
                ExchangeOrderID{1},
                SecurityID{0},
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            market_events::BookOrderDeletion{
                ExchangeOrderID{1},
                SecurityID{0},
            },
            market_events::BookOrderEntry{
                ExchangeOrderID{2},
                SecurityID{0},
                Direction::BUY,
                Price{2},
                Amount{1},
            },
            market_events::BookOrderDeletion{
                ExchangeOrderID{2},
                SecurityID{0},
            },
        },
    .expected_reports =
        {
            report::OrderAccepted{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            report::OrderBookEntry{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                Direction::BUY,
                Price{1},
                Amount{1},
            },
            report::ReplacementAccepted{
                ExchangeOrderID{1},
                ExchangeOrderID{2},
                ClientID{0},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
                Direction::BUY,
                Price{2},
                Amount{1},
            },
            report::OrderBookEntry{
                ExchangeOrderID{2},
                ClientID{0},
                ClientOrderID{1},
                SecurityID{0},
                Direction::BUY,
                Price{2},
                Amount{1},
            },
            report::OrderCancelled{
                ExchangeOrderID{2},
                ClientID{0},
                ClientOrderID{1},
                ClientOrderID{2},
                SecurityID{0},
            },
        },
    .test_name = "AddLimit_Replace_Cancel",
};

const MatchingOutputTestInput ReplaceLosesPriority_IocTradesOrder2ThenReplacementThenExpires = {
    .requests =
        {
            request::OrderAddRequest{
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                OrderType::LIMIT,
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            request::OrderAddRequest{
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                OrderType::LIMIT,
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            request::OrderReplaceRequest{
                ClientID{0},
                ClientOrderID{0},
                ClientOrderID{2},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            request::OrderAddRequest{
                ClientID{2},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::IOC,
                OrderType::LIMIT,
                Direction::BUY,
                Price{10},
                Amount{3},
            },
        },
    .expected_market_data =
        {
            market_events::BookOrderEntry{
                ExchangeOrderID{1},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            market_events::BookOrderEntry{
                ExchangeOrderID{2},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            market_events::BookOrderDeletion{
                ExchangeOrderID{1},
                SecurityID{0},
            },
            market_events::BookOrderEntry{
                ExchangeOrderID{3},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            market_events::Trade{
                SecurityID{0},
                Price{10},
                Amount{1},
                ExchangeTradeID{1},
                ExchangeOrderID{4},
                ExchangeOrderID{2},
            },
            market_events::BookOrderDeletion{
                ExchangeOrderID{2},
                SecurityID{0},
            },
            market_events::Trade{
                SecurityID{0},
                Price{10},
                Amount{1},
                ExchangeTradeID{2},
                ExchangeOrderID{4},
                ExchangeOrderID{3},
            },
            market_events::BookOrderDeletion{
                ExchangeOrderID{3},
                SecurityID{0},
            },
        },
    .expected_reports =
        {
            report::OrderAccepted{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            report::OrderBookEntry{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            report::OrderAccepted{
                ExchangeOrderID{2},
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            report::OrderBookEntry{
                ExchangeOrderID{2},
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            report::ReplacementAccepted{
                ExchangeOrderID{1},
                ExchangeOrderID{3},
                ClientID{0},
                ClientOrderID{0},
                ClientOrderID{2},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            report::OrderBookEntry{
                ExchangeOrderID{3},
                ClientID{0},
                ClientOrderID{2},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            report::OrderAccepted{
                ExchangeOrderID{4},
                ClientID{2},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::IOC,
                Direction::BUY,
                Price{10},
                Amount{3},
            },
            report::Trade{
                SecurityID{0},
                Direction::BUY,
                Price{10},
                Amount{1},
                ClientID{2},
                ClientOrderID{0},
                ExchangeTradeID{1},
                ExchangeOrderID{4},
                ExchangeOrderID{2},
            },
            report::Trade{
                SecurityID{0},
                Direction::BUY,
                Price{10},
                Amount{1},
                ClientID{1},
                ClientOrderID{0},
                ExchangeTradeID{1},
                ExchangeOrderID{4},
                ExchangeOrderID{2},
            },
            report::OrderFilled{
                ExchangeOrderID{2},
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                engine_events::FilledType::PASSIVE_FILL,
            },
            report::Trade{
                SecurityID{0},
                Direction::BUY,
                Price{10},
                Amount{1},
                ClientID{2},
                ClientOrderID{0},
                ExchangeTradeID{2},
                ExchangeOrderID{4},
                ExchangeOrderID{3},
            },
            report::Trade{
                SecurityID{0},
                Direction::BUY,
                Price{10},
                Amount{1},
                ClientID{0},
                ClientOrderID{2},
                ExchangeTradeID{2},
                ExchangeOrderID{4},
                ExchangeOrderID{3},
            },
            report::OrderFilled{
                ExchangeOrderID{3},
                ClientID{0},
                ClientOrderID{2},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                engine_events::FilledType::PASSIVE_FILL,
            },
            report::OrderExpired{
                ExchangeOrderID{4},
                ClientID{2},
                ClientOrderID{0},
                SecurityID{0},
                engine_events::ExpirationReason::IOC_REMAINDER_EXPIRED,
            },
        },
    .test_name = "ReplaceLosesPriority_IocTradesOrder2ThenReplacementThenExpires",
};

const MatchingOutputTestInput IocFullyTradesBestPricePartiallyTradesNextPrice_CancelRemaining = {
    .requests =
        {
            request::OrderAddRequest{
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                OrderType::LIMIT,
                Direction::SELL,
                Price{10},
                Amount{2},
            },
            request::OrderAddRequest{
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                OrderType::LIMIT,
                Direction::SELL,
                Price{11},
                Amount{3},
            },
            request::OrderAddRequest{
                ClientID{2},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::IOC,
                OrderType::LIMIT,
                Direction::BUY,
                Price{11},
                Amount{3},
            },
            request::OrderCancelRequest{
                ClientID{1},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
            },
        },
    .expected_market_data =
        {
            market_events::BookOrderEntry{
                ExchangeOrderID{1},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{2},
            },
            market_events::BookOrderEntry{
                ExchangeOrderID{2},
                SecurityID{0},
                Direction::SELL,
                Price{11},
                Amount{3},
            },
            market_events::Trade{
                SecurityID{0},
                Price{10},
                Amount{2},
                ExchangeTradeID{1},
                ExchangeOrderID{3},
                ExchangeOrderID{1},
            },
            market_events::BookOrderDeletion{
                ExchangeOrderID{1},
                SecurityID{0},
            },
            market_events::Trade{
                SecurityID{0},
                Price{11},
                Amount{1},
                ExchangeTradeID{2},
                ExchangeOrderID{3},
                ExchangeOrderID{2},
            },
            market_events::BookOrderChange{
                ExchangeOrderID{2},
                SecurityID{0},
                Direction::SELL,
                Price{11},
                Amount{2},
            },
            market_events::BookOrderDeletion{
                ExchangeOrderID{2},
                SecurityID{0},
            },
        },
    .expected_reports =
        {
            report::OrderAccepted{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                Direction::SELL,
                Price{10},
                Amount{2},
            },
            report::OrderBookEntry{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{2},
            },
            report::OrderAccepted{
                ExchangeOrderID{2},
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                Direction::SELL,
                Price{11},
                Amount{3},
            },
            report::OrderBookEntry{
                ExchangeOrderID{2},
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                Direction::SELL,
                Price{11},
                Amount{3},
            },
            report::OrderAccepted{
                ExchangeOrderID{3},
                ClientID{2},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::IOC,
                Direction::BUY,
                Price{11},
                Amount{3},
            },
            report::Trade{
                SecurityID{0},
                Direction::BUY,
                Price{10},
                Amount{2},
                ClientID{2},
                ClientOrderID{0},
                ExchangeTradeID{1},
                ExchangeOrderID{3},
                ExchangeOrderID{1},
            },
            report::Trade{
                SecurityID{0},
                Direction::BUY,
                Price{10},
                Amount{2},
                ClientID{0},
                ClientOrderID{0},
                ExchangeTradeID{1},
                ExchangeOrderID{3},
                ExchangeOrderID{1},
            },
            report::OrderFilled{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                engine_events::FilledType::PASSIVE_FILL,
            },
            report::Trade{
                SecurityID{0},
                Direction::BUY,
                Price{11},
                Amount{1},
                ClientID{2},
                ClientOrderID{0},
                ExchangeTradeID{2},
                ExchangeOrderID{3},
                ExchangeOrderID{2},
            },
            report::Trade{
                SecurityID{0},
                Direction::BUY,
                Price{11},
                Amount{1},
                ClientID{1},
                ClientOrderID{0},
                ExchangeTradeID{2},
                ExchangeOrderID{3},
                ExchangeOrderID{2},
            },
            report::OrderFilled{
                ExchangeOrderID{3},
                ClientID{2},
                ClientOrderID{0},
                SecurityID{0},
                Direction::BUY,
                Price{11},
                engine_events::FilledType::AGGRESSIVE_FILL,
            },
            report::OrderCancelled{
                ExchangeOrderID{2},
                ClientID{1},
                ClientOrderID{0},
                ClientOrderID{1},
                SecurityID{0},
            },
        },
    .test_name = "IocFullyTradesBestPricePartiallyTradesNextPrice_CancelRemaining",
};

const MatchingOutputTestInput RejectedReplaceKeepsPriority_IocTradesOrder1ThenOrder2ThenExpires = {
    .requests =
        {
            request::OrderAddRequest{
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                OrderType::LIMIT,
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            request::OrderAddRequest{
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                OrderType::LIMIT,
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            request::OrderReplaceRequest{
                ClientID{0},
                ClientOrderID{0},
                ClientOrderID{2},
                SecurityID{0},
                Direction::SELL,
                Price{101},
                Amount{1},
            },
            request::OrderAddRequest{
                ClientID{2},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::IOC,
                OrderType::LIMIT,
                Direction::BUY,
                Price{10},
                Amount{3},
            },
        },
    .expected_market_data =
        {
            market_events::BookOrderEntry{
                ExchangeOrderID{1},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            market_events::BookOrderEntry{
                ExchangeOrderID{2},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            market_events::Trade{
                SecurityID{0},
                Price{10},
                Amount{1},
                ExchangeTradeID{1},
                ExchangeOrderID{3},
                ExchangeOrderID{1},
            },
            market_events::BookOrderDeletion{
                ExchangeOrderID{1},
                SecurityID{0},
            },
            market_events::Trade{
                SecurityID{0},
                Price{10},
                Amount{1},
                ExchangeTradeID{2},
                ExchangeOrderID{3},
                ExchangeOrderID{2},
            },
            market_events::BookOrderDeletion{
                ExchangeOrderID{2},
                SecurityID{0},
            },
        },
    .expected_reports =
        {
            report::OrderAccepted{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            report::OrderBookEntry{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            report::OrderAccepted{
                ExchangeOrderID{2},
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::LIMIT,
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            report::OrderBookEntry{
                ExchangeOrderID{2},
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                Amount{1},
            },
            report::ReplacementRejected{
                ClientID{0},
                ClientOrderID{0},
                ClientOrderID{2},
                SecurityID{0},
                report::ReplacementRejectedReason::PRICE_OUT_OF_RANGE,
            },
            report::OrderAccepted{
                ExchangeOrderID{3},
                ClientID{2},
                ClientOrderID{0},
                SecurityID{0},
                OrderTimeInForce::IOC,
                Direction::BUY,
                Price{10},
                Amount{3},
            },
            report::Trade{
                SecurityID{0},
                Direction::BUY,
                Price{10},
                Amount{1},
                ClientID{2},
                ClientOrderID{0},
                ExchangeTradeID{1},
                ExchangeOrderID{3},
                ExchangeOrderID{1},
            },
            report::Trade{
                SecurityID{0},
                Direction::BUY,
                Price{10},
                Amount{1},
                ClientID{0},
                ClientOrderID{0},
                ExchangeTradeID{1},
                ExchangeOrderID{3},
                ExchangeOrderID{1},
            },
            report::OrderFilled{
                ExchangeOrderID{1},
                ClientID{0},
                ClientOrderID{0},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                engine_events::FilledType::PASSIVE_FILL,
            },
            report::Trade{
                SecurityID{0},
                Direction::BUY,
                Price{10},
                Amount{1},
                ClientID{2},
                ClientOrderID{0},
                ExchangeTradeID{2},
                ExchangeOrderID{3},
                ExchangeOrderID{2},
            },
            report::Trade{
                SecurityID{0},
                Direction::BUY,
                Price{10},
                Amount{1},
                ClientID{1},
                ClientOrderID{0},
                ExchangeTradeID{2},
                ExchangeOrderID{3},
                ExchangeOrderID{2},
            },
            report::OrderFilled{
                ExchangeOrderID{2},
                ClientID{1},
                ClientOrderID{0},
                SecurityID{0},
                Direction::SELL,
                Price{10},
                engine_events::FilledType::PASSIVE_FILL,
            },
            report::OrderExpired{
                ExchangeOrderID{3},
                ClientID{2},
                ClientOrderID{0},
                SecurityID{0},
                engine_events::ExpirationReason::IOC_REMAINDER_EXPIRED,
            },
        },
    .test_name = "RejectedReplaceKeepsPriority_IocTradesOrder1ThenOrder2ThenExpires",
};

INSTANTIATE_TEST_SUITE_P(MatchingOutputTest,
    MatchingOutputTest,
    ::testing::Values(AddLimit_Cancel,
        AddIoc_Expire,
        Cancel_Reject,
        Replace_Reject,
        AddLimitTwoClientsSameClientOrderId_CancelSecond_CancelFirst,
        AddLimit_Replace_Cancel,
        ReplaceLosesPriority_IocTradesOrder2ThenReplacementThenExpires,
        IocFullyTradesBestPricePartiallyTradesNextPrice_CancelRemaining,
        RejectedReplaceKeepsPriority_IocTradesOrder1ThenOrder2ThenExpires),
    [](const ::testing::TestParamInfo<MatchingOutputTestInput>& info) { return info.param.test_name; });
