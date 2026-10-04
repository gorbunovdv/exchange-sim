#include "base/callback_sink.h"

#include "exchange/book_containers/stl_book_container.h"
#include "exchange/matching/matching_engine.h"

#include <glaze/glaze.hpp>
#include <gtest/gtest.h>

using namespace exchange_sim;

TEST(MatchingEngineTest, UnavailableSecurityTest) {
  auto compare_event = []<typename VariantType>(const VariantType& actual, const auto& expected) {
    ASSERT_TRUE(std::holds_alternative<std::remove_cvref_t<decltype(expected)>>(actual));
    EXPECT_EQ(expected, std::get<std::remove_cvref_t<decltype(expected)>>(actual));
  };

  std::vector<engine_events::EventType> events;
  CallbackSink event_sink([&events](engine_events::EventType&& event) { events.push_back(event); });
  SecurityStorage storage({
      SecurityConfig{
          .symbol = Symbol{"A"},
          .tick_size = Price(1),
          .lot_size = Amount(1),
          .low_price = Price(0),
          .high_price = Price(100),
          .max_amount = Amount(1000),
      },
  });

  using MatchingEngineType = MatchingEngine<StlL3BookContainer, decltype(event_sink)>;

  MatchingEngineType engine(MatchingEngineType::Config{.reserve_books_size = 128}, storage, event_sink);
  engine.process(request::OrderAddRequest{
      .origin_time = Ticks{0},
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
      .origin_time = Ticks{0},
      .client_id = ClientID{0},
      .client_order_id = ClientOrderID{0},
      .client_replacement_id = ClientOrderID{1},
      .security_id = SecurityID{1},
      .direction = Direction::BUY,
      .price = Price(2),
      .quantity = Amount(1),
  });
  engine.process(request::OrderCancelRequest{
      .origin_time = Ticks{0},
      .client_id = ClientID{0},
      .client_order_id = ClientOrderID{1},
      .client_cancellation_id = ClientOrderID{2},
      .security_id = SecurityID{1},
  });
  ASSERT_TRUE(events.size() == 3);
  compare_event(events[0],
      (engine_events::OrderRejected{
          .origin_time = Ticks{0},
          .security_id = SecurityID{1},
          .client_id = ClientID{0},
          .client_order_id = ClientOrderID{0},
          .reason = engine_events::OrderRejectedReason::SECURITY_NOT_FOUND,
      }));
  compare_event(events[1],
      (engine_events::ReplacementRejected{
          .origin_time = Ticks{0},
          .client_id = ClientID{0},
          .client_order_id = ClientOrderID{0},
          .client_replacement_id = ClientOrderID{1},
          .security_id = SecurityID{1},
          .reason = engine_events::ReplacementRejectedReason::SECURITY_NOT_FOUND,
      }));
  compare_event(events[2],
      (engine_events::CancellationRejected{
          .origin_time = Ticks{0},
          .client_id = ClientID{0},
          .client_order_id = ClientOrderID{1},
          .client_cancellation_id = ClientOrderID{2},
          .security_id = SecurityID{1},
          .reason = engine_events::CancellationRejectedReason::SECURITY_NOT_FOUND,
      }));
}
