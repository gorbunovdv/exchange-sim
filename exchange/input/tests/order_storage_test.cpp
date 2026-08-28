#include "base/one_of.h"

#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"
#include "exchange/input/order_storage.h"

#include <boost/hof/match.hpp>
#include <gtest/gtest.h>

using namespace exchange_sim;

using Order = OrderStorage::Order;

void bring_order_to_state(OrderStorage& storage, request::OrderAddRequest request, Order::State state) {
  storage.handle(request);
  if (state == Order::State::SENT) {
    return;
  }
  storage.handle(report::OrderAccepted{
      .origin_time = request.origin_time,
      .order_id = ExchangeOrderID{request.client_order_id.value()},
      .client_id = request.client_id,
      .client_order_id = request.client_order_id,
      .security_id = request.security_id,
      .time_in_force = request.time_in_force,
      .direction = request.direction,
      .price = request.price,
      .quantity = request.quantity,
  });
  if (state == Order::State::ACCEPTED) {
    return;
  }
  if (request.time_in_force == OrderTimeInForce::IOC) {
    CHECK(state != Order::State::ACTIVE, "IOC order can't be active");
  } else {
    storage.handle(report::OrderBookEntry{
        .origin_time = request.origin_time,
        .order_id = ExchangeOrderID{request.client_order_id.value()},
        .client_id = request.client_id,
        .client_order_id = request.client_order_id,
        .security_id = request.security_id,
        .direction = request.direction,
        .price = request.price,
        .quantity = request.quantity,
    });
    if (state == Order::State::ACTIVE) {
      return;
    }
    if (state == Order::State::CANCEL_SENT) {
      storage.handle(request::OrderCancelRequest{
          .origin_time = request.origin_time,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .client_cancellation_id = ClientOrderID{request.client_order_id.value() + 1},
          .security_id = request.security_id,
      });
    } else if (state == Order::State::REPLACE_SENT) {
      storage.handle(request::OrderReplaceRequest{
          .origin_time = request.origin_time,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .client_replacement_id = ClientOrderID{request.client_order_id.value() + 1},
          .security_id = request.security_id,
          .direction = request.direction,
          .price = request.price,
          .quantity = request.quantity,
      });
    } else {
      CRITICAL("Order state is not handled: {}", format_as(state));
    }
  }
}

static constexpr request::OrderAddRequest constOrderAddRequest = {
    .origin_time = Ticks{0},
    .client_id = ClientID{0},
    .client_order_id = ClientOrderID{0},
    .security_id = SecurityID{0},
    .time_in_force = OrderTimeInForce::LIMIT,
    .order_type = OrderType::LIMIT,
    .direction = Direction::BUY,
    .price = Price{0},
    .quantity = Amount{1},
};

template<typename T>
T convert_to(const request::OrderAddRequest& request);

template<>
report::OrderAccepted convert_to<report::OrderAccepted>(const request::OrderAddRequest& request) {
  return report::OrderAccepted{
      .origin_time = request.origin_time,
      .order_id = ExchangeOrderID{request.client_order_id.value()},
      .client_id = request.client_id,
      .client_order_id = request.client_order_id,
      .security_id = request.security_id,
      .time_in_force = request.time_in_force,
      .direction = request.direction,
      .price = request.price,
      .quantity = request.quantity,
  };
}

template<>
report::OrderBookEntry convert_to<report::OrderBookEntry>(const request::OrderAddRequest& request) {
  return report::OrderBookEntry{
      .origin_time = request.origin_time,
      .order_id = ExchangeOrderID{request.client_order_id.value()},
      .client_id = request.client_id,
      .client_order_id = request.client_order_id,
      .security_id = request.security_id,
      .direction = request.direction,
      .price = request.price,
      .quantity = request.quantity,
  };
}

template<>
report::OrderFilled convert_to<report::OrderFilled>(const request::OrderAddRequest& request) {
  return report::OrderFilled{
      .origin_time = request.origin_time,
      .order_id = ExchangeOrderID{request.client_order_id.value()},
      .client_id = request.client_id,
      .client_order_id = request.client_order_id,
      .security_id = request.security_id,
      .direction = request.direction,
      .price = request.price,
      .type = engine_events::FilledType::AGGRESSIVE_FILL,
  };
}

template<>
report::OrderExpired convert_to<report::OrderExpired>(const request::OrderAddRequest& request) {
  return report::OrderExpired{
      .origin_time = request.origin_time,
      .order_id = ExchangeOrderID{request.client_order_id.value()},
      .client_id = request.client_id,
      .client_order_id = request.client_order_id,
      .security_id = request.security_id,
      .reason = engine_events::ExpirationReason::IOC_REMAINDER_EXPIRED,
  };
}

template<>
report::OrderCancelled convert_to<report::OrderCancelled>(const request::OrderAddRequest& request) {
  return report::OrderCancelled{
      .origin_time = request.origin_time,
      .order_id = ExchangeOrderID{request.client_order_id.value()},
      .client_id = request.client_id,
      .client_order_id = request.client_order_id,
      .client_cancellation_id = ClientOrderID{request.client_order_id.value() + 1},
      .security_id = request.security_id,
  };
}

template<>
report::OrderRejected convert_to<report::OrderRejected>(const request::OrderAddRequest& request) {
  return report::OrderRejected{
      .origin_time = request.origin_time,
      .security_id = request.security_id,
      .client_id = request.client_id,
      .client_order_id = request.client_order_id,
      .reason = engine_events::OrderRejectedReason::SECURITY_NOT_FOUND,
  };
}

template<>
report::ReplacementAccepted convert_to<report::ReplacementAccepted>(const request::OrderAddRequest& request) {
  return report::ReplacementAccepted{
      .origin_time = request.origin_time,
      .order_id = ExchangeOrderID{request.client_order_id.value() + 1},
      .new_order_id = ExchangeOrderID{request.client_order_id.value() + 1},
      .client_id = request.client_id,
      .client_order_id = request.client_order_id,
      .client_replacement_id = ClientOrderID{request.client_order_id.value() + 1},
      .security_id = request.security_id,
      .direction = request.direction,
      .price = request.price,
      .quantity = request.quantity,
  };
}

template<>
report::CancellationRejected convert_to<report::CancellationRejected>(const request::OrderAddRequest& request) {
  return report::CancellationRejected{
      .origin_time = request.origin_time,
      .client_id = request.client_id,
      .client_order_id = request.client_order_id,
      .client_cancellation_id = ClientOrderID{request.client_order_id.value() + 1},
      .security_id = request.security_id,
      .reason = engine_events::CancellationRejectedReason::SECURITY_NOT_FOUND,
  };
}

template<>
report::ReplacementRejected convert_to<report::ReplacementRejected>(const request::OrderAddRequest& request) {
  return report::ReplacementRejected{
      .origin_time = request.origin_time,
      .client_id = request.client_id,
      .client_order_id = request.client_order_id,
      .client_replacement_id = ClientOrderID{request.client_order_id.value() + 1},
      .security_id = request.security_id,
      .reason = engine_events::ReplacementRejectedReason::SECURITY_NOT_FOUND,
  };
}

void apply_events(OrderStorage& storage, const std::vector<report::ReportType>& events) {
  for (const auto& event : events) {
    std::visit(
        boost::hof::match([](const report::Trade&) {}, [&](const auto& typed_event) { storage.handle(typed_event); }),
        event);
  }
}

struct ExpectedOrder {
  Order::State state;
};

struct ExpectedOrderStorage {
  std::optional<ExpectedOrder> order;
};

struct TransitionTestInput {
  Order::State initial_state;
  std::vector<report::ReportType> events;
  ExpectedOrderStorage expected_storage;
  std::string test_name;
};

std::ostream& operator<<(std::ostream& os, const TransitionTestInput& input) {
  return os << input.test_name;
}

class OrderStorageTransitionTest : public ::testing::TestWithParam<TransitionTestInput> {};

TEST_P(OrderStorageTransitionTest, TransitionApplication) {
  auto expect_storage = [](const OrderStorage& storage, const ExpectedOrderStorage& expected) {
    size_t active_orders = (expected.order.has_value() ? (expected.order.value().state == Order::State::ACTIVE) : 0);
    size_t pending_transactions = (expected.order.has_value() ? equal_to_one_of(expected.order.value().state,
                                                                    Order::State::SENT,
                                                                    Order::State::ACCEPTED,
                                                                    Order::State::CANCEL_SENT,
                                                                    Order::State::REPLACE_SENT) :
                                                                0);
    EXPECT_EQ(active_orders, storage.active_orders().size());
    EXPECT_EQ(pending_transactions, storage.pending_transactions());
    if (expected.order) {
      ASSERT_TRUE(storage.orders().size() == 1);
      const auto& order = storage.orders().begin()->second;
      EXPECT_EQ(order.state, expected.order->state);
    }
  };

  const auto& input = GetParam();

  OrderStorage storage;

  bring_order_to_state(storage, constOrderAddRequest, input.initial_state);

  apply_events(storage, input.events);
  expect_storage(storage, input.expected_storage);
}

class OrderStorageTransitionFailureTest : public ::testing::TestWithParam<TransitionTestInput> {};

TEST_P(OrderStorageTransitionFailureTest, TransitionFailure) {
  const auto& input = GetParam();
  OrderStorage storage;

  bring_order_to_state(storage, constOrderAddRequest, input.initial_state);

  ASSERT_THROW(apply_events(storage, input.events), std::runtime_error);
}

template<typename EventType, Order::State State, Order::State ExpectedState>
TransitionTestInput transition_test(const std::string& test_name) {
  return TransitionTestInput{
      .initial_state = State,
      .events = convert_to<EventType>(constOrderAddRequest),
      .expected_storage = {ExpectedState},
      .test_name = test_name,
  };
}

INSTANTIATE_TEST_SUITE_P(OrderStorageTransitions,
    OrderStorageTransitionTest,
    ::testing::Values(
        TransitionTestInput{
            .initial_state = Order::State::SENT,
            .events = {report::ReportType{convert_to<report::OrderAccepted>(constOrderAddRequest)}},
            .expected_storage = {.order = ExpectedOrder{.state = Order::State::ACCEPTED}},
            .test_name = "Sent_OrderAccepted",
        },
        TransitionTestInput{
            .initial_state = Order::State::SENT,
            .events = {report::ReportType{convert_to<report::OrderRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Sent_OrderRejected",
        },

        TransitionTestInput{
            .initial_state = Order::State::ACCEPTED,
            .events = {report::ReportType{convert_to<report::OrderBookEntry>(constOrderAddRequest)}},
            .expected_storage = {.order = ExpectedOrder{.state = Order::State::ACTIVE}},
            .test_name = "Accepted_OrderBookEntry",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACCEPTED,
            .events = {report::ReportType{convert_to<report::OrderFilled>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Accepted_OrderFilled",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACCEPTED,
            .events = {report::ReportType{convert_to<report::OrderExpired>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Accepted_OrderExpired",
        },

        TransitionTestInput{
            .initial_state = Order::State::ACTIVE,
            .events = {report::ReportType{convert_to<report::OrderFilled>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Active_OrderFilled",
        },

        TransitionTestInput{
            .initial_state = Order::State::CANCEL_SENT,
            .events = {report::ReportType{convert_to<report::OrderCancelled>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "CancelSent_OrderCancelled",
        },
        TransitionTestInput{
            .initial_state = Order::State::CANCEL_SENT,
            .events = {report::ReportType{convert_to<report::CancellationRejected>(constOrderAddRequest)}},
            .expected_storage = {.order = ExpectedOrder{.state = Order::State::ACTIVE}},
            .test_name = "CancelSent_CancellationRejected",
        },

        TransitionTestInput{
            .initial_state = Order::State::REPLACE_SENT,
            .events = {report::ReportType{convert_to<report::ReplacementAccepted>(constOrderAddRequest)}},
            .expected_storage = {.order = ExpectedOrder{.state = Order::State::ACCEPTED}},
            .test_name = "ReplaceSent_ReplacementAccepted",
        },
        TransitionTestInput{
            .initial_state = Order::State::REPLACE_SENT,
            .events = {report::ReportType{convert_to<report::ReplacementRejected>(constOrderAddRequest)}},
            .expected_storage = {.order = ExpectedOrder{.state = Order::State::ACTIVE}},
            .test_name = "ReplaceSent_ReplacementRejected",
        },

        TransitionTestInput{
            .initial_state = Order::State::CANCEL_SENT,
            .events =
                {
                    report::ReportType{convert_to<report::OrderFilled>(constOrderAddRequest)},
                    report::ReportType{convert_to<report::CancellationRejected>(constOrderAddRequest)},
                },
            .expected_storage = {},
            .test_name = "CancelSent_OrderFilled_CancelRejected",
        },
        TransitionTestInput{
            .initial_state = Order::State::REPLACE_SENT,
            .events =
                {
                    report::ReportType{convert_to<report::OrderFilled>(constOrderAddRequest)},
                    report::ReportType{convert_to<report::ReplacementRejected>(constOrderAddRequest)},
                },
            .expected_storage = {},
            .test_name = "ReplaceSent_OrderFilled_ReplaceRejected",
        }),
    [](const ::testing::TestParamInfo<TransitionTestInput>& info) { return info.param.test_name; });


INSTANTIATE_TEST_SUITE_P(OrderStorageTransitionFailures,
    OrderStorageTransitionFailureTest,
    ::testing::Values(
        TransitionTestInput{
            .initial_state = Order::State::SENT,
            .events = {report::ReportType{convert_to<report::OrderBookEntry>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Sent_OrderBookEntry",
        },
        TransitionTestInput{
            .initial_state = Order::State::SENT,
            .events = {report::ReportType{convert_to<report::OrderFilled>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Sent_OrderFilled",
        },
        TransitionTestInput{
            .initial_state = Order::State::SENT,
            .events = {report::ReportType{convert_to<report::OrderExpired>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Sent_OrderExpired",
        },
        TransitionTestInput{
            .initial_state = Order::State::SENT,
            .events = {report::ReportType{convert_to<report::OrderCancelled>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Sent_OrderCancelled",
        },
        TransitionTestInput{
            .initial_state = Order::State::SENT,
            .events = {report::ReportType{convert_to<report::CancellationRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Sent_CancellationRejected",
        },
        TransitionTestInput{
            .initial_state = Order::State::SENT,
            .events = {report::ReportType{convert_to<report::ReplacementAccepted>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Sent_ReplacementAccepted",
        },
        TransitionTestInput{
            .initial_state = Order::State::SENT,
            .events = {report::ReportType{convert_to<report::ReplacementRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Sent_ReplacementRejected",
        },

        TransitionTestInput{
            .initial_state = Order::State::ACCEPTED,
            .events = {report::ReportType{convert_to<report::OrderAccepted>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Accepted_OrderAccepted",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACCEPTED,
            .events = {report::ReportType{convert_to<report::OrderCancelled>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Accepted_OrderCancelled",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACCEPTED,
            .events = {report::ReportType{convert_to<report::OrderRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Accepted_OrderRejected",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACCEPTED,
            .events = {report::ReportType{convert_to<report::CancellationRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Accepted_CancellationRejected",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACCEPTED,
            .events = {report::ReportType{convert_to<report::ReplacementAccepted>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Accepted_ReplacementAccepted",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACCEPTED,
            .events = {report::ReportType{convert_to<report::ReplacementRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Accepted_ReplacementRejected",
        },

        TransitionTestInput{
            .initial_state = Order::State::ACTIVE,
            .events = {report::ReportType{convert_to<report::OrderAccepted>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Active_OrderAccepted",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACTIVE,
            .events = {report::ReportType{convert_to<report::OrderBookEntry>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Active_OrderBookEntry",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACTIVE,
            .events = {report::ReportType{convert_to<report::OrderExpired>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Active_OrderExpired",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACTIVE,
            .events = {report::ReportType{convert_to<report::OrderCancelled>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Active_OrderCancelled",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACTIVE,
            .events = {report::ReportType{convert_to<report::OrderRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Active_OrderRejected",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACTIVE,
            .events = {report::ReportType{convert_to<report::CancellationRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Active_CancellationRejected",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACTIVE,
            .events = {report::ReportType{convert_to<report::ReplacementAccepted>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Active_ReplacementAccepted",
        },
        TransitionTestInput{
            .initial_state = Order::State::ACTIVE,
            .events = {report::ReportType{convert_to<report::ReplacementRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "Active_ReplacementRejected",
        },

        TransitionTestInput{
            .initial_state = Order::State::CANCEL_SENT,
            .events = {report::ReportType{convert_to<report::OrderAccepted>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "CancelSent_OrderAccepted",
        },
        TransitionTestInput{
            .initial_state = Order::State::CANCEL_SENT,
            .events = {report::ReportType{convert_to<report::OrderBookEntry>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "CancelSent_OrderBookEntry",
        },
        TransitionTestInput{
            .initial_state = Order::State::CANCEL_SENT,
            .events = {report::ReportType{convert_to<report::OrderExpired>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "CancelSent_OrderExpired",
        },
        TransitionTestInput{
            .initial_state = Order::State::CANCEL_SENT,
            .events = {report::ReportType{convert_to<report::OrderRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "CancelSent_OrderRejected",
        },
        TransitionTestInput{
            .initial_state = Order::State::CANCEL_SENT,
            .events = {report::ReportType{convert_to<report::ReplacementAccepted>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "CancelSent_ReplacementAccepted",
        },
        TransitionTestInput{
            .initial_state = Order::State::CANCEL_SENT,
            .events = {report::ReportType{convert_to<report::ReplacementRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "CancelSent_ReplacementRejected",
        },

        TransitionTestInput{
            .initial_state = Order::State::REPLACE_SENT,
            .events = {report::ReportType{convert_to<report::OrderAccepted>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "ReplaceSent_OrderAccepted",
        },
        TransitionTestInput{
            .initial_state = Order::State::REPLACE_SENT,
            .events = {report::ReportType{convert_to<report::OrderBookEntry>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "ReplaceSent_OrderBookEntry",
        },
        TransitionTestInput{
            .initial_state = Order::State::REPLACE_SENT,
            .events = {report::ReportType{convert_to<report::OrderExpired>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "ReplaceSent_OrderExpired",
        },
        TransitionTestInput{
            .initial_state = Order::State::REPLACE_SENT,
            .events = {report::ReportType{convert_to<report::OrderCancelled>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "ReplaceSent_OrderCancelled",
        },
        TransitionTestInput{
            .initial_state = Order::State::REPLACE_SENT,
            .events = {report::ReportType{convert_to<report::OrderRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "ReplaceSent_OrderRejected",
        },
        TransitionTestInput{
            .initial_state = Order::State::REPLACE_SENT,
            .events = {report::ReportType{convert_to<report::CancellationRejected>(constOrderAddRequest)}},
            .expected_storage = {},
            .test_name = "ReplaceSent_CancellationRejected",
        }),
    [](const ::testing::TestParamInfo<TransitionTestInput>& info) { return info.param.test_name; });
