#include "base/callback_sink.h"

#include "exchange/matching/matching_engine.h"

#include <glaze/glaze.hpp>
#include <gtest/gtest.h>

using namespace exchange_sim;

TEST(MatchingEngineTest, UnavailableSecurityTest) {
  auto compare_event = []<typename VariantType>(const VariantType& actual, const auto& expected) {
    ASSERT_TRUE(std::holds_alternative<std::remove_cvref_t<decltype(expected)>>(actual));
    EXPECT_EQ(expected, std::get<std::remove_cvref_t<decltype(expected)>>(actual));
  };

  std::vector<engine_snapshot::SnapshotType> snapshots;
  CallbackSink<engine_snapshot::SnapshotType> snapshot_sink(
      [&snapshots](engine_snapshot::SnapshotType&& snapshot) { snapshots.push_back(std::move(snapshot)); });
  SecurityStorage storage({
      SecurityConfig{
          .symbol = "A",
          .tick_size = Price(1),
          .lot_size = Amount(1),
          .low_price = Price(0),
          .high_price = Price(100),
          .max_amount = Amount(1000),
      },
  });
  MatchingEngine engine(glz::generic{}, storage, snapshot_sink);
  engine.process(request::OrderAddRequest{
      .client_id = ClientID{0},
      .client_order_id = ClientOrderID{0},
      .security_id = SecurityID{1},
      .time_in_force = OrderTimeInForce::IOC,
      .order_type = OrderType::LIMIT,
      .direction = Direction::BUY,
      .price = Price(1),
      .quantity = Amount(1),
  });
  engine.process(request::OrderReplaceRequest{
      .client_id = ClientID{0},
      .client_order_id = ClientOrderID{0},
      .client_replacement_id = ClientOrderID{1},
      .security_id = SecurityID{1},
      .direction = Direction::BUY,
      .price = Price(2),
      .quantity = Amount(1),
  });
  engine.process(request::OrderCancelRequest{
      .client_id = ClientID{0},
      .client_order_id = ClientOrderID{1},
      .client_cancellation_id = ClientOrderID{2},
      .security_id = SecurityID{1},
  });
  ASSERT_TRUE(snapshots.size() == 3);
  compare_event(snapshots[0],
      (engine_snapshot::OrderRejected{
          .security_id = SecurityID{1},
          .client_id = ClientID{0},
          .client_order_id = ClientOrderID{0},
          .reason = engine_snapshot::OrderRejectedReason::SECURITY_NOT_FOUND,
      }));
  compare_event(snapshots[1],
      (engine_snapshot::ReplacementRejected{
          .client_id = ClientID{0},
          .client_order_id = ClientOrderID{0},
          .client_replacement_id = ClientOrderID{1},
          .security_id = SecurityID{1},
          .reason = engine_snapshot::ReplacementRejectedReason::SECURITY_NOT_FOUND,
      }));
  compare_event(snapshots[2],
      (engine_snapshot::CancellationRejected{
          .client_id = ClientID{0},
          .client_order_id = ClientOrderID{1},
          .client_cancellation_id = ClientOrderID{2},
          .security_id = SecurityID{1},
          .reason = engine_snapshot::CancellationRejectedReason::SECURITY_NOT_FOUND,
      }));
}