#pragma once

#include <random>

#include "base/logging/loggable_class.h"
#include "application/events.h"
#include "application/security.h"

namespace exchange_sim {

template<template<typename> class SinkType>
class SimpleOrderGenerator : public LoggableClass {
public:
  explicit SimpleOrderGenerator(
      SecurityStorage& security_storage, const glz::generic& config, SinkType<RequestType>& matching_engine_sink):
      LoggableClass("SimpleOrderGenerator"),
      matching_engine_sink_(matching_engine_sink),
      min_active_orders_by_security_(config["min_active_orders_by_security"].as<std::uint32_t>()),
      max_active_orders_by_security_(config["max_active_orders_by_security"].as<std::uint32_t>()),
      security_storage_(security_storage),
      active_orders_(security_storage_.size()),
      generator_(config["seed"].as<std::uint32_t>()),
      security_id_distribution_(0, security_storage_.size() - 1) {}

  void run() {
    while (!asked_to_stop()) {
      auto order = generate_event();
      if (std::holds_alternative<OrderAddRequest>(order) &&
          std::get<OrderAddRequest>(order).time_in_force == OrderTimeInForce::LIMIT) {
        active_orders_[std::get<OrderAddRequest>(order).security_id].emplace_back(
            ActiveOrder{std::get<OrderAddRequest>(order).client_order_id});
      }
      if (std::holds_alternative<OrderCancelRequest>(order)) {
        SecurityID security_id = std::get<OrderCancelRequest>(order).security_id;
        active_orders_[security_id].erase(
            std::remove_if(active_orders_[security_id].begin(),
                active_orders_[security_id].end(),
                [order_id = std::get<OrderCancelRequest>(order).client_order_id](
                    const ActiveOrder& active_order) { return active_order.order_id == order_id; }),
            active_orders_[std::get<OrderCancelRequest>(order).security_id].end());
      }
      matching_engine_sink_.push(order);
    }
  }

  bool asked_to_stop() const { return false; }

private:
  RequestType generate_event() {
    if (auto it = std::ranges::find_if(
            active_orders_, [this](const auto& pair) { return pair.size() >= max_active_orders_by_security_; });
        it != active_orders_.end()) {
      return generate_cancel_order_request(static_cast<SecurityID>(it - active_orders_.begin()));
    }
    if (auto it = std::ranges::find_if(
            active_orders_, [this](auto& pair) { return pair.size() < min_active_orders_by_security_; });
        it != active_orders_.end()) {
      return generate_new_order_request(static_cast<SecurityID>(it - active_orders_.begin()));
    }

    auto type = type_distribution_(generator_);
    if (type == 0) {
      return generate_new_order_request(security_id_distribution_(generator_));
    }
    if (type == 1) {
      return generate_cancel_order_request(security_id_distribution_(generator_));
    }
    return generate_modify_order_request(security_id_distribution_(generator_));
  }

  OrderAddRequest generate_new_order_request(SecurityID security_id) {
    const Security& security = security_storage_.get_security(security_id);
    Price price_interval = security.high_price - security.low_price;
    int64_t price_max_multiplied_value = static_cast<uint64_t>(price_interval / security.tick_size);

    Price resulting_price =
        std::uniform_int_distribution<uint64_t>(0, price_max_multiplied_value)(generator_) * security.tick_size +
        security.low_price;
    Amount resulting_amount = std::uniform_int_distribution<uint64_t>(
                                  0, static_cast<uint64_t>(security.max_amount / security.lot_size))(generator_) *
        security.lot_size;
    OrderTimeInForce time_in_force = magic_enum::enum_value<OrderTimeInForce>(
        std::uniform_int_distribution<uint32_t>(0, magic_enum::enum_count<OrderTimeInForce>() - 1)(generator_));
    Direction direction = magic_enum::enum_value<Direction>(
        std::uniform_int_distribution<uint32_t>(0, magic_enum::enum_count<Direction>() - 1)(generator_));

    return OrderAddRequest{
        .client_id = 0,
        .client_order_id = current_client_order_id_++,
        .security_id = security_id,
        .time_in_force = time_in_force,
        .order_type = OrderType::LIMIT,
        .direction = direction,
        .price = resulting_price,
        .quantity = resulting_amount,
    };
  }

  OrderCancelRequest generate_cancel_order_request(SecurityID security_id) {
    const auto& active_orders = active_orders_.at(security_id);
    size_t order_to_cancel_index = std::uniform_int_distribution<size_t>(0, active_orders.size() - 1)(generator_);

    return OrderCancelRequest{
        .client_id = 0,
        .client_order_id = active_orders[order_to_cancel_index].order_id,
        .security_id = security_id,
    };
  }

  OrderModifyRequest generate_modify_order_request(SecurityID security_id) {
    const auto& active_orders = active_orders_.at(security_id);
    size_t order_to_modify_index = std::uniform_int_distribution<size_t>(0, active_orders.size() - 1)(generator_);

    const Security& security = security_storage_.get_security(security_id);
    Price price_interval = security.high_price - security.low_price;
    int64_t price_max_multiplied_value = static_cast<uint64_t>(price_interval / security.tick_size);

    Price resulting_price =
        std::uniform_int_distribution<uint64_t>(0, price_max_multiplied_value)(generator_) * security.tick_size +
        security.low_price;
    Amount resulting_amount = std::uniform_int_distribution<uint64_t>(
                                  0, static_cast<uint64_t>(security.max_amount / security.lot_size))(generator_) *
        security.lot_size;

    Direction direction = magic_enum::enum_value<Direction>(
        std::uniform_int_distribution<uint32_t>(0, magic_enum::enum_count<Direction>() - 1)(generator_));

    return OrderModifyRequest{
        .client_id = 0,
        .client_order_id = active_orders[order_to_modify_index].order_id,
        .security_id = security_id,
        .direction = direction,
        .price = resulting_price,
        .quantity = resulting_amount,
    };
  }

  struct ActiveOrder {
    ClientOrderID order_id;
  };

  SinkType<RequestType>& matching_engine_sink_;

  const uint32_t min_active_orders_by_security_;
  const uint32_t max_active_orders_by_security_;

  SecurityStorage& security_storage_;
  std::vector<std::vector<ActiveOrder>> active_orders_;

  std::mt19937 generator_;
  std::uniform_int_distribution<std::uint32_t> type_distribution_{0, 3};
  std::uniform_int_distribution<SecurityID> security_id_distribution_;

  uint32_t current_client_order_id_ = 0;
};

}  // namespace exchange_sim