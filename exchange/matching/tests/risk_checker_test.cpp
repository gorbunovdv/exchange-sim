#include "exchange/matching/risk_checker.h"

#include <gtest/gtest.h>

using namespace exchange_sim;

namespace exchange_sim {

static const auto security = Security{
    .security_id = SecurityID{0},
    .symbol = "A",
    .tick_size = Price(0.2),
    .lot_size = Amount(0.1),
    .low_price = Price(1),
    .high_price = Amount(10),
    .max_amount = Amount(100),
};
static const auto risk_checker = RiskChecker(security);

TEST(RiskChecker, CorrectOrderTest) {
  EXPECT_EQ(risk_checker.reject_request(request::OrderAddRequest{
                .client_id = 0,
                .client_order_id = 0,
                .security_id = SecurityID{0},
                .time_in_force = OrderTimeInForce::IOC,
                .order_type = OrderType::LIMIT,
                .direction = Direction::BUY,
                .price = Price(1.2),
                .quantity = Amount(1),
            }),
      std::nullopt);
}

TEST(RiskChecker, CorrectReplaceTest) {
  EXPECT_EQ(risk_checker.reject_request(request::OrderReplaceRequest{
                .client_id = 0,
                .client_order_id = 0,
                .client_replacement_id = 0,
                .security_id = SecurityID{0},
                .direction = Direction::BUY,
                .price = Price(1),
                .quantity = Amount(1),
            }),
      std::nullopt);
}

TEST(RiskChecker, TooMuchAmountOrderTest) {
  EXPECT_EQ(risk_checker.reject_request(request::OrderAddRequest{
                .client_id = 0,
                .client_order_id = 0,
                .security_id = SecurityID{0},
                .time_in_force = OrderTimeInForce::IOC,
                .order_type = OrderType::LIMIT,
                .direction = Direction::BUY,
                .price = Price(1),
                .quantity = Amount(10000),
            }),
      engine_snapshot::OrderRejectedReason::MAX_AMOUNT_EXCEEDED);
}

TEST(RiskChecker, TooMuchAmountReplaceTest) {
  EXPECT_EQ(risk_checker.reject_request(request::OrderReplaceRequest{
                .client_id = 0,
                .client_order_id = 0,
                .client_replacement_id = 0,
                .security_id = SecurityID{0},
                .direction = Direction::BUY,
                .price = Price(1),
                .quantity = Amount(10000),
            }),
      engine_snapshot::ReplacementRejectedReason::MAX_AMOUNT_EXCEEDED);
}

TEST(RiskChecker, InvalidAmountOrderTest) {
  EXPECT_EQ(risk_checker.reject_request(request::OrderAddRequest{
                .client_id = 0,
                .client_order_id = 0,
                .security_id = SecurityID{0},
                .time_in_force = OrderTimeInForce::IOC,
                .order_type = OrderType::LIMIT,
                .direction = Direction::BUY,
                .price = Price(1),
                .quantity = Amount(0),
            }),
      engine_snapshot::OrderRejectedReason::INVALID_QUANTITY);

  EXPECT_EQ(risk_checker.reject_request(request::OrderAddRequest{
                .client_id = 0,
                .client_order_id = 0,
                .security_id = SecurityID{0},
                .time_in_force = OrderTimeInForce::IOC,
                .order_type = OrderType::LIMIT,
                .direction = Direction::BUY,
                .price = Price(1),
                .quantity = Amount(-1),
            }),
      engine_snapshot::OrderRejectedReason::INVALID_QUANTITY);

  EXPECT_EQ(risk_checker.reject_request(request::OrderAddRequest{
                .client_id = 0,
                .client_order_id = 0,
                .security_id = SecurityID{0},
                .time_in_force = OrderTimeInForce::IOC,
                .order_type = OrderType::LIMIT,
                .direction = Direction::BUY,
                .price = Price(1),
                .quantity = Amount(1.05),
            }),
      engine_snapshot::OrderRejectedReason::INVALID_QUANTITY);
}

TEST(RiskChecker, InvalidAmountReplaceTest) {
  EXPECT_EQ(risk_checker.reject_request(request::OrderReplaceRequest{
                .client_id = 0,
                .client_order_id = 0,
                .client_replacement_id = 0,
                .security_id = SecurityID{0},
                .direction = Direction::BUY,
                .price = Price(1),
                .quantity = Amount(0),
            }),
      engine_snapshot::ReplacementRejectedReason::INVALID_QUANTITY);

  EXPECT_EQ(risk_checker.reject_request(request::OrderReplaceRequest{
                .client_id = 0,
                .client_order_id = 0,
                .client_replacement_id = 0,
                .security_id = SecurityID{0},
                .direction = Direction::BUY,
                .price = Price(1),
                .quantity = Amount(-1),
            }),
      engine_snapshot::ReplacementRejectedReason::INVALID_QUANTITY);

  EXPECT_EQ(risk_checker.reject_request(request::OrderReplaceRequest{
                .client_id = 0,
                .client_order_id = 0,
                .client_replacement_id = 0,
                .security_id = SecurityID{0},
                .direction = Direction::BUY,
                .price = Price(1),
                .quantity = Amount(1.05),
            }),
      engine_snapshot::ReplacementRejectedReason::INVALID_QUANTITY);
}

TEST(RiskChecker, PriceOutOfRangeOrderTest) {
  EXPECT_EQ(risk_checker.reject_request(request::OrderAddRequest{
                .client_id = 0,
                .client_order_id = 0,
                .security_id = SecurityID{0},
                .time_in_force = OrderTimeInForce::IOC,
                .order_type = OrderType::LIMIT,
                .direction = Direction::BUY,
                .price = Price(0.5),
                .quantity = Amount(1),
            }),
      engine_snapshot::OrderRejectedReason::PRICE_OUT_OF_RANGE);

  EXPECT_EQ(risk_checker.reject_request(request::OrderAddRequest{
                .client_id = 0,
                .client_order_id = 0,
                .security_id = SecurityID{0},
                .time_in_force = OrderTimeInForce::IOC,
                .order_type = OrderType::LIMIT,
                .direction = Direction::BUY,
                .price = Price(20),
                .quantity = Amount(1),
            }),
      engine_snapshot::OrderRejectedReason::PRICE_OUT_OF_RANGE);
}

TEST(RiskChecker, PriceOutOfRangeReplaceTest) {
  EXPECT_EQ(risk_checker.reject_request(request::OrderReplaceRequest{
                .client_id = 0,
                .client_order_id = 0,
                .client_replacement_id = 0,
                .security_id = SecurityID{0},
                .direction = Direction::BUY,
                .price = Price(0.5),
                .quantity = Amount(1),
            }),
      engine_snapshot::ReplacementRejectedReason::PRICE_OUT_OF_RANGE);

  EXPECT_EQ(risk_checker.reject_request(request::OrderReplaceRequest{
                .client_id = 0,
                .client_order_id = 0,
                .client_replacement_id = 0,
                .security_id = SecurityID{0},
                .direction = Direction::BUY,
                .price = Price(20),
                .quantity = Amount(1),
            }),
      engine_snapshot::ReplacementRejectedReason::PRICE_OUT_OF_RANGE);
}

TEST(RiskChecker, InvalidPriceTickSizeOrderTest) {
  EXPECT_EQ(risk_checker.reject_request(request::OrderAddRequest{
                .client_id = 0,
                .client_order_id = 0,
                .security_id = SecurityID{0},
                .time_in_force = OrderTimeInForce::IOC,
                .order_type = OrderType::LIMIT,
                .direction = Direction::BUY,
                .price = Price(1.1),
                .quantity = Amount(1),
            }),
      engine_snapshot::OrderRejectedReason::PRICE_NOT_MULTIPLE_OF_TICK_SIZE);
}

TEST(RiskChecker, InvalidPriceTickSizeReplaceTest) {
  EXPECT_EQ(risk_checker.reject_request(request::OrderReplaceRequest{
                .client_id = 0,
                .client_order_id = 0,
                .client_replacement_id = 0,
                .security_id = SecurityID{0},
                .direction = Direction::BUY,
                .price = Price(1.1),
                .quantity = Amount(1),
            }),
      engine_snapshot::ReplacementRejectedReason::PRICE_NOT_MULTIPLE_OF_TICK_SIZE);
}

}  // namespace exchange_sim