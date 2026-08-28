#pragma once

#include "base/counter.h"

#include "domain/security.h"

#include "exchange/book_containers/book_container_concept.h"
#include "exchange/domain/requests.h"
#include "exchange/matching/risk_checker.h"

namespace exchange_sim {

template<template<typename> class BookContainerType>
class L3Book {
public:
  explicit L3Book(
      const Security& security, Counter<ExchangeOrderID>& order_id_counter, Counter<ExchangeTradeID>& trade_id_counter):
      security_(security),
      risk_checker_(security),
      order_id_counter_(order_id_counter),
      trade_id_counter_(trade_id_counter) {}

  void add(const request::OrderAddRequest& request, engine_events::Sink auto& sink) {
    if (request.direction == Direction::BUY) {
      add_impl<Direction::BUY>(request, sink);
    } else {
      add_impl<Direction::SELL>(request, sink);
    }
  }

  void replace(const request::OrderReplaceRequest& request, engine_events::Sink auto& sink) {
    if (request.direction == Direction::BUY) {
      replace_impl<Direction::BUY>(request, sink);
    } else {
      replace_impl<Direction::SELL>(request, sink);
    }
  }

  void cancel(const request::OrderCancelRequest& request, engine_events::Sink auto& sink) {
    if (auto order_opt = book_container_.extract_order(std::pair{request.client_id, request.client_order_id});
        order_opt.has_value()) {
      auto order = order_opt.value();
      sink.push(engine_events::OrderCancelled{
          .origin_time = request.origin_time,
          .order_id = order.order_id,
          .client_id = request.client_id,
          .client_order_id = order.client_order_id,
          .client_cancellation_id = request.client_cancellation_id,
          .security_id = security_.security_id,
      });
    } else {
      sink.push(engine_events::CancellationRejected{
          .origin_time = request.origin_time,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .client_cancellation_id = request.client_cancellation_id,
          .security_id = security_.security_id,
          .reason = engine_events::CancellationRejectedReason::ORDER_NOT_FOUND,
      });
    }
  }

private:
  struct ActiveOrder {
    Ticks origin_time;
    Direction direction;
    ExchangeOrderID order_id;
    ClientID client_id;
    ClientOrderID client_order_id;
    Price price;
    Amount quantity;

    auto id() const { return std::pair{client_id, client_order_id}; }
  };

  static_assert(BookContainer<BookContainerType<ActiveOrder>>);

  template<Direction current_order_direction>
  void add_impl(const request::OrderAddRequest& request, engine_events::Sink auto& sink) {
    if (auto outcome = risk_checker_.reject_request(request); outcome.has_value()) {
      sink.push(engine_events::OrderRejected{
          .origin_time = request.origin_time,
          .security_id = security_.security_id,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .reason = outcome.value(),
      });
      return;
    }
    if (book_container_.contains(std::pair{request.client_id, request.client_order_id})) {
      sink.push(engine_events::OrderRejected{
          .origin_time = request.origin_time,
          .security_id = security_.security_id,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .reason = engine_events::OrderRejectedReason::DUPLICATE_CLIENT_ORDER_ID,
      });
      return;
    }
    ActiveOrder current_order = {
        .origin_time = request.origin_time,
        .direction = request.direction,
        .order_id = generate_order_id(),
        .client_id = request.client_id,
        .client_order_id = request.client_order_id,
        .price = request.price,
        .quantity = request.quantity,
    };
    sink.push(engine_events::OrderAccepted{
        .origin_time = request.origin_time,
        .order_id = current_order.order_id,
        .client_id = current_order.client_id,
        .client_order_id = current_order.client_order_id,
        .security_id = security_.security_id,
        .time_in_force = request.time_in_force,
        .direction = current_order.direction,
        .price = current_order.price,
        .quantity = current_order.quantity,
    });
    current_order = fill_order_and_place_in_book<current_order_direction>(current_order, request.time_in_force, sink);
    if (current_order.quantity != Amount{0} && request.time_in_force != OrderTimeInForce::IOC) {
      sink.push(engine_events::OrderBookEntry{
          .origin_time = request.origin_time,
          .order_id = current_order.order_id,
          .client_id = current_order.client_id,
          .client_order_id = current_order.client_order_id,
          .security_id = security_.security_id,
          .direction = request.direction,
          .price = request.price,
          .quantity = current_order.quantity,
      });
    } else if (current_order.quantity == Amount{0}) {
      sink.push(engine_events::OrderFilled{
          .origin_time = request.origin_time,
          .order_id = current_order.order_id,
          .client_id = current_order.client_id,
          .client_order_id = current_order.client_order_id,
          .security_id = security_.security_id,
          .direction = current_order.direction,
          .price = current_order.price,
          .type = engine_events::FilledType::AGGRESSIVE_FILL,
      });
    } else {
      sink.push(engine_events::OrderExpired{
          .origin_time = request.origin_time,
          .order_id = current_order.order_id,
          .client_id = current_order.client_id,
          .client_order_id = current_order.client_order_id,
          .security_id = security_.security_id,
          .reason = engine_events::ExpirationReason::IOC_REMAINDER_EXPIRED,
      });
    }
  }

  template<Direction current_order_direction>
  void replace_impl(const request::OrderReplaceRequest& request, engine_events::Sink auto& sink) {
    if (auto outcome = risk_checker_.reject_request(request); outcome.has_value()) {
      sink.push(engine_events::ReplacementRejected{
          .origin_time = request.origin_time,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .client_replacement_id = request.client_replacement_id,
          .security_id = security_.security_id,
          .reason = outcome.value(),
      });
      return;
    }
    if (request.client_order_id != request.client_replacement_id &&
        book_container_.contains(std::pair{request.client_id, request.client_replacement_id})) {
      sink.push(engine_events::ReplacementRejected{
          .origin_time = request.origin_time,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .client_replacement_id = request.client_replacement_id,
          .security_id = security_.security_id,
          .reason = engine_events::ReplacementRejectedReason::DUPLICATE_CLIENT_ORDER_ID,
      });
      return;
    }
    if (auto current_order_opt = book_container_.extract_order(std::pair{request.client_id, request.client_order_id});
        current_order_opt.has_value()) {
      auto current_order = current_order_opt.value();
      ExchangeOrderID new_order_id = generate_order_id();
      sink.push(engine_events::ReplacementAccepted{
          .origin_time = request.origin_time,
          .order_id = current_order.order_id,
          .new_order_id = new_order_id,
          .client_id = current_order.client_id,
          .client_order_id = current_order.client_order_id,
          .client_replacement_id = request.client_replacement_id,
          .security_id = security_.security_id,
          .direction = request.direction,
          .price = request.price,
          .quantity = request.quantity,
      });
      current_order.origin_time = request.origin_time;
      current_order.order_id = new_order_id;
      current_order.direction = request.direction;
      current_order.price = request.price;
      current_order.quantity = request.quantity;
      current_order.client_order_id = request.client_replacement_id;
      current_order =
          fill_order_and_place_in_book<current_order_direction>(current_order, OrderTimeInForce::LIMIT, sink);
      if (current_order.quantity != Amount{0}) {
        sink.push(engine_events::OrderBookEntry{
            .origin_time = request.origin_time,
            .order_id = current_order.order_id,
            .client_id = current_order.client_id,
            .client_order_id = current_order.client_order_id,
            .security_id = security_.security_id,
            .direction = request.direction,
            .price = request.price,
            .quantity = current_order.quantity,
        });
      } else {
        sink.push(engine_events::OrderFilled{
            .origin_time = request.origin_time,
            .order_id = current_order.order_id,
            .client_id = request.client_id,
            .client_order_id = current_order.client_order_id,
            .security_id = security_.security_id,
            .direction = current_order.direction,
            .price = current_order.price,
            .type = engine_events::FilledType::AGGRESSIVE_FILL,
        });
      }
    } else {
      sink.push(engine_events::ReplacementRejected{
          .origin_time = request.origin_time,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .client_replacement_id = request.client_replacement_id,
          .security_id = security_.security_id,
          .reason = engine_events::ReplacementRejectedReason::ORDER_NOT_FOUND,
      });
    }
  }

  template<Direction current_order_direction>
  ActiveOrder fill_order_and_place_in_book(
      ActiveOrder current_order, OrderTimeInForce time_in_force, engine_events::Sink auto& sink) {
    constexpr Direction opposite_current_order_direction =
        current_order_direction == Direction::BUY ? Direction::SELL : Direction::BUY;
    auto& opposite_direction_book = book_container_.template opposite_half<current_order_direction>();
    while (current_order.quantity > Amount{0} && !opposite_direction_book.empty() &&
        !opposite_direction_book.key_comp()(current_order.price, opposite_direction_book.begin()->first)) {
      current_order = fill_order_with_level<opposite_current_order_direction>(
          current_order, opposite_direction_book.begin()->second, sink);
      if (opposite_direction_book.begin()->second.empty()) {
        opposite_direction_book.erase(opposite_direction_book.begin());
      }
    }
    if (current_order.quantity != Amount{0} && time_in_force != OrderTimeInForce::IOC) {
      book_container_.template push_back<current_order_direction>(current_order);
    }
    return current_order;
  }

  template<Direction opposite_current_order_direction>
  [[nodiscard]]
  ActiveOrder fill_order_with_level(
      ActiveOrder current_order, auto& opposite_direction_book_level, engine_events::Sink auto& sink) {
    while (current_order.quantity > Amount{0} && !opposite_direction_book_level.empty()) {
      auto& opposite_order = opposite_direction_book_level.front();
      Amount quantity_to_fill = std::min(current_order.quantity, opposite_order.quantity);
      current_order.quantity -= quantity_to_fill;
      opposite_order.quantity -= quantity_to_fill;
      sink.push(engine_events::Trade{
          .origin_time = current_order.origin_time,
          .security_id = security_.security_id,
          .aggressor_direction = current_order.direction,
          .price = opposite_order.price,
          .quantity = quantity_to_fill,
          .trade_id = generate_trade_id(),
          .aggressor_order_id = current_order.order_id,
          .aggressor_client_id = current_order.client_id,
          .aggressor_client_order_id = current_order.client_order_id,
          .recipient_order_id = opposite_order.order_id,
          .recipient_client_id = opposite_order.client_id,
          .recipient_client_order_id = opposite_order.client_order_id,
      });
      if (opposite_order.quantity == Amount{0}) {
        sink.push(engine_events::OrderFilled{
            .origin_time = current_order.origin_time,
            .order_id = opposite_order.order_id,
            .client_id = opposite_order.client_id,
            .client_order_id = opposite_order.client_order_id,
            .security_id = security_.security_id,
            .direction = opposite_order.direction,
            .price = opposite_order.price,
            .type = engine_events::FilledType::PASSIVE_FILL,
        });
        book_container_.template erase_front<opposite_current_order_direction>();
      } else {
        sink.push(engine_events::OrderReduced{
            .origin_time = current_order.origin_time,
            .order_id = opposite_order.order_id,
            .client_id = opposite_order.client_id,
            .client_order_id = opposite_order.client_order_id,
            .security_id = security_.security_id,
            .direction = opposite_order.direction,
            .price = opposite_order.price,
            .remaining_quantity = opposite_order.quantity,
        });
      }
    }
    return current_order;
  }

  ExchangeOrderID generate_order_id() { return order_id_counter_.inc(); }

  ExchangeTradeID generate_trade_id() { return trade_id_counter_.inc(); }

  const Security& security_;
  RiskChecker risk_checker_;
  BookContainerType<ActiveOrder> book_container_;

  Counter<ExchangeOrderID>& order_id_counter_;
  Counter<ExchangeTradeID>& trade_id_counter_;
};

}  // namespace exchange_sim

template<>
struct std::hash<std::pair<exchange_sim::ClientID, exchange_sim::ClientOrderID>> {
  size_t operator()(const std::pair<exchange_sim::ClientID, exchange_sim::ClientOrderID>& pair) const noexcept {
    return std::hash<uint64_t>{}(((std::hash<exchange_sim::ClientID>{}(pair.first) << 32) |
        std::hash<exchange_sim::ClientOrderID>{}(pair.second)));
  }
};