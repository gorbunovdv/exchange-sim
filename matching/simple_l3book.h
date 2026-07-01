#pragma once

#include "base/counter.h"
#include "domain/security.h"
#include "domain/requests.h"
#include "matching/risk_checker.h"
#include <map>
#include <list>

namespace exchange_sim {

class SimpleL3Book {
public:
  explicit SimpleL3Book(
      const Security& security, Counter<ExchangeOrderID>& order_id_counter, Counter<ExchangeTradeID>& trade_id_counter);

  void add(request::OrderAddRequest request, auto& sink) {
    if (auto outcome = risk_checker_.reject_request(request); outcome.has_value()) {
      sink.push(engine_snapshot::OrderRejected{
          .security_id = security_.security_id,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .reason = outcome.value(),
      });
      return;
    }
    if (active_orders_.contains(std::make_pair(request.client_id, request.client_order_id))) {
      sink.push(engine_snapshot::OrderRejected{
          .security_id = security_.security_id,
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .reason = engine_snapshot::OrderRejectedReason::DUPLICATE_CLIENT_ORDER_ID,
      });
      return;
    }
    ActiveOrder current_order = {
        .direction = request.direction,
        .order_id = generate_order_id(),
        .client_id = request.client_id,
        .client_order_id = request.client_order_id,
        .price = request.price,
        .quantity = request.quantity,
    };
    sink.push(engine_snapshot::OrderAccepted{
        .order_id = current_order.order_id,
        .client_id = current_order.client_id,
        .client_order_id = current_order.client_order_id,
        .security_id = security_.security_id,
        .time_in_force = request.time_in_force,
        .direction = current_order.direction,
        .price = current_order.price,
        .quantity = current_order.quantity,
    });
    current_order = fill_order_and_place_in_book(current_order, request.time_in_force, sink);
    if (current_order.quantity != 0 && request.time_in_force != OrderTimeInForce::IOC) {
      sink.push(engine_snapshot::OrderBookEntry{
          .order_id = current_order.order_id,
          .client_id = current_order.client_id,
          .client_order_id = current_order.client_order_id,
          .security_id = security_.security_id,
          .direction = request.direction,
          .price = request.price,
          .quantity = current_order.quantity,
      });
    } else if (current_order.quantity == 0) {
      sink.push(engine_snapshot::OrderFilled{
          .order_id = current_order.order_id,
          .client_id = current_order.client_id,
          .client_order_id = current_order.client_order_id,
          .security_id = security_.security_id,
          .direction = current_order.direction,
          .price = current_order.price,
          .remaining_quantity = current_order.quantity,
          .type = engine_snapshot::FilledType::AGGRESSIVE_FILL,
          .fully_filled = true,
      });
    } else {
      sink.push(engine_snapshot::OrderExpired{
          .order_id = current_order.order_id,
          .client_id = current_order.client_id,
          .client_order_id = current_order.client_order_id,
          .security_id = security_.security_id,
          .reason = engine_snapshot::ExpirationReason::IOC_REMAINDER_EXPIRED,
      });
    }
  }

  void modify(request::OrderModifyRequest request, auto& sink) {
    if (auto outcome = risk_checker_.reject_request(request); outcome.has_value()) {
      sink.push(engine_snapshot::ModificationRejected{
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .client_modification_id = request.client_modification_id,
          .security_id = security_.security_id,
          .reason = outcome.value(),
      });
      return;
    }
    if (request.client_order_id != request.client_modification_id &&
        active_orders_.contains(std::make_pair(request.client_id, request.client_modification_id))) {
      sink.push(engine_snapshot::ModificationRejected{.client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .client_modification_id = request.client_modification_id,
          .security_id = security_.security_id,
          .reason = engine_snapshot::ModificationRejectedReason::DUPLICATE_CLIENT_ORDER_ID});
      return;
    }
    if (auto current_order_opt = extract_order(request.client_id, request.client_order_id);
        current_order_opt.has_value()) {
      auto current_order = current_order_opt.value();
      ExchangeOrderID new_order_id = generate_order_id();
      sink.push(engine_snapshot::ModificationAccepted{
          .order_id = current_order.order_id,
          .new_order_id = new_order_id,
          .client_id = current_order.client_id,
          .client_order_id = current_order.client_order_id,
          .client_modification_id = request.client_modification_id,
          .security_id = security_.security_id,
          .direction = request.direction,
          .price = request.price,
          .quantity = request.quantity,
      });
      current_order.order_id = new_order_id;
      current_order.direction = request.direction;
      current_order.price = request.price;
      current_order.quantity = request.quantity;
      current_order.client_order_id = request.client_modification_id;
      current_order = fill_order_and_place_in_book(current_order, OrderTimeInForce::LIMIT, sink);
      if (current_order.quantity != 0) {
        sink.push(engine_snapshot::OrderBookEntry{
            .order_id = current_order.order_id,
            .client_id = current_order.client_id,
            .client_order_id = current_order.client_order_id,
            .security_id = security_.security_id,
            .direction = request.direction,
            .price = request.price,
            .quantity = current_order.quantity,
        });
      } else {
        sink.push(engine_snapshot::OrderFilled{.order_id = current_order.order_id,
            .client_id = request.client_id,
            .client_order_id = current_order.client_order_id,
            .security_id = security_.security_id,
            .direction = current_order.direction,
            .price = current_order.price,
            .remaining_quantity = current_order.quantity,
            .type = engine_snapshot::FilledType::AGGRESSIVE_FILL,
            .fully_filled = true});
      }
    } else {
      sink.push(engine_snapshot::ModificationRejected{
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .client_modification_id = request.client_modification_id,
          .security_id = security_.security_id,
          .reason = engine_snapshot::ModificationRejectedReason::ORDER_NOT_FOUND,
      });
    }
  }

  void cancel(request::OrderCancelRequest request, auto& sink) {
    if (auto order_opt = extract_order(request.client_id, request.client_order_id); order_opt.has_value()) {
      auto order = order_opt.value();
      sink.push(engine_snapshot::OrderCancelled{
          .order_id = order.order_id,
          .client_id = request.client_id,
          .client_order_id = order.client_order_id,
          .client_cancellation_id = request.client_cancellation_id,
          .security_id = security_.security_id,
      });
    } else {
      sink.push(engine_snapshot::CancellationRejected{
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .client_cancellation_id = request.client_cancellation_id,
          .security_id = security_.security_id,
          .reason = engine_snapshot::CancellationRejectedReason::ORDER_NOT_FOUND,
      });
    }
  }

private:
  struct ActiveOrder {
    Direction direction;
    ExchangeOrderID order_id;
    ClientID client_id;
    ClientOrderID client_order_id;
    Price price;
    Amount quantity;
  };

  std::optional<ActiveOrder> extract_order(ClientID client_id, ClientOrderID client_order_id);

  ActiveOrder fill_order_and_place_in_book(ActiveOrder current_order, OrderTimeInForce time_in_force, auto& sink) {
    Direction opposite_direction = current_order.direction == Direction::BUY ? Direction::SELL : Direction::BUY;
    auto& direction_book = book_[magic_enum::enum_integer(current_order.direction)];
    auto& opposite_direction_book = book_[magic_enum::enum_integer(opposite_direction)];
    if (current_order.direction == Direction::BUY) {
      while (current_order.quantity > 0 && !opposite_direction_book.empty() &&
          opposite_direction_book.begin()->first <= current_order.price) {
        current_order = fill_order_with_level(current_order, opposite_direction_book.begin()->second, sink);
        if (opposite_direction_book.begin()->second.empty()) {
          opposite_direction_book.erase(opposite_direction_book.begin());
        }
      }
    } else if (current_order.direction == Direction::SELL) {
      while (current_order.quantity > 0 && !opposite_direction_book.empty() &&
          opposite_direction_book.rbegin()->first >= current_order.price) {
        current_order = fill_order_with_level(current_order, opposite_direction_book.rbegin()->second, sink);
        if (opposite_direction_book.rbegin()->second.empty()) {
          opposite_direction_book.erase(std::prev(opposite_direction_book.end()));
        }
      }
    }
    if (current_order.quantity != 0 && time_in_force != OrderTimeInForce::IOC) {
      direction_book[current_order.price].emplace_back(current_order);
      active_orders_.emplace(std::make_pair(current_order.client_id, current_order.client_order_id),
          std::prev(direction_book.at(current_order.price).end()));
    }
    return current_order;
  }

  [[nodiscard]]
  ActiveOrder fill_order_with_level(
      ActiveOrder current_order, std::list<ActiveOrder>& opposite_direction_book_level, auto& sink) {
    while (current_order.quantity > 0 && !opposite_direction_book_level.empty()) {
      Amount quantity_to_fill = std::min(current_order.quantity, opposite_direction_book_level.front().quantity);
      current_order.quantity -= quantity_to_fill;
      opposite_direction_book_level.front().quantity -= quantity_to_fill;
      sink.push(engine_snapshot::Trade{
          .security_id = security_.security_id,
          .aggressor_direction = current_order.direction,
          .price = opposite_direction_book_level.front().price,
          .quantity = quantity_to_fill,
          .trade_id = generate_trade_id(),
          .aggressor_order_id = current_order.order_id,
          .aggressor_client_id = current_order.client_id,
          .aggressor_client_order_id = current_order.client_order_id,
          .recipient_order_id = opposite_direction_book_level.front().order_id,
          .recipient_client_id = opposite_direction_book_level.front().client_id,
          .recipient_client_order_id = opposite_direction_book_level.front().client_order_id,
      });
      sink.push(engine_snapshot::OrderFilled{
          .order_id = opposite_direction_book_level.front().order_id,
          .client_id = opposite_direction_book_level.front().client_id,
          .client_order_id = opposite_direction_book_level.front().client_order_id,
          .security_id = security_.security_id,
          .direction = opposite_direction_book_level.front().direction,
          .price = opposite_direction_book_level.front().price,
          .remaining_quantity = opposite_direction_book_level.front().quantity,
          .type = engine_snapshot::FilledType::PASSIVE_FILL,
          .fully_filled = opposite_direction_book_level.front().quantity == 0,
      });
      if (opposite_direction_book_level.front().quantity == 0) {
        active_orders_.erase(std::make_pair(
            opposite_direction_book_level.front().client_id, opposite_direction_book_level.front().client_order_id));
        opposite_direction_book_level.pop_front();
      }
    }
    return current_order;
  }

  ExchangeOrderID generate_order_id() { return order_id_counter_.inc(); }

  ExchangeTradeID generate_trade_id() { return trade_id_counter_.inc(); }

  const Security& security_;
  RiskChecker risk_checker_;
  std::array<std::map<Price, std::list<ActiveOrder>>, magic_enum::enum_count<Direction>()> book_;
  std::unordered_map<std::pair<ClientID, ClientOrderID>,
      std::list<ActiveOrder>::iterator,
      decltype([](const std::pair<ClientID, ClientOrderID>& key) {
        return std::hash<ClientID>()(key.first) ^ std::hash<ClientOrderID>()(key.second);
      })>
      active_orders_;

  Counter<ExchangeOrderID>& order_id_counter_;
  Counter<ExchangeTradeID>& trade_id_counter_;
};

}  // namespace exchange_sim