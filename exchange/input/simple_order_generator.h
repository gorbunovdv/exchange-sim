#pragma once

#include "base/logging/loggable_class.h"
#include "base/random.h"

#include "domain/security.h"

#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/input/store_orders_skill.h"

#include <glaze/glaze.hpp>

namespace exchange_sim {

template<typename GeneratorInterface>
  requires(std::derived_from<GeneratorInterface, generator_interface_skills::StoreOrders>)
class SimpleOrderGenerator : public LoggableClass {
public:
  explicit SimpleOrderGenerator(GeneratorInterface& interface, const glz::generic& config):
      LoggableClass("SimpleOrderGenerator"),
      interface_(interface),
      min_active_orders_by_security_(config["min_active_orders_by_security"].as<std::uint32_t>()),
      max_active_orders_by_security_(config["max_active_orders_by_security"].as<std::uint32_t>()),
      max_pending_transactions_(config["max_pending_transactions"].as<std::uint32_t>()),
      random_(config["seed"].as<std::uint32_t>()) {
    CHECK(min_active_orders_by_security_ <= max_active_orders_by_security_,
        "Min active orders must be less than max, min_active_orders_by_security={} max_active_orders_by_security={}",
        min_active_orders_by_security_,
        max_active_orders_by_security_);
    CHECK(min_active_orders_by_security_ > 0,
        "Min active orders must be greater than 0, min_active_orders_by_security={}",
        min_active_orders_by_security_);
  }

  void generate_event() {
    if (interface_.pending_transactions() > max_pending_transactions_) {
      return;
    }
    if (auto it = std::ranges::find_if(interface_.order_storage(),
            [this](const auto& pair) { return pair.active_orders().size() >= max_active_orders_by_security_; });
        it != interface_.order_storage().end()) {
      generate_cancel_order_request(static_cast<SecurityID>(it - interface_.order_storage().begin()));
      return;
    }
    if (auto it = std::ranges::find_if(interface_.order_storage(),
            [this](auto& pair) { return pair.active_orders().size() < min_active_orders_by_security_; });
        it != interface_.order_storage().end()) {
      generate_new_order_request(static_cast<SecurityID>(it - interface_.order_storage().begin()));
      return;
    }

    auto type = random_.rand_int(0, 2);
    auto security = random_.rand_int_strong_type<SecurityID>(0, interface_.security_storage().size() - 1);
    if (type == 0) {
      generate_new_order_request(security);
      return;
    }
    if (type == 1) {
      generate_cancel_order_request(security);
      return;
    }
    generate_replace_order_request(security);
  }

  void process_market_event(const auto&) {}
  void process_report(const auto&) {}

private:
  void generate_new_order_request(SecurityID security_id) {
    const Security& security = interface_.security_storage().get_security(security_id);
    auto resulting_price = random_.rand_decimal<Price>(security.low_price, security.high_price, security.tick_size);
    auto resulting_amount = random_.rand_decimal<Amount>(security.lot_size, security.max_amount, security.lot_size);
    auto time_in_force = random_.rand_enum<OrderTimeInForce>();
    auto direction = random_.rand_enum<Direction>();

    interface_.send_order_add_request(
        ClientID{0}, security_id, time_in_force, OrderType::LIMIT, direction, resulting_price, resulting_amount);
  }

  void generate_cancel_order_request(SecurityID security_id) {
    CHECK(!interface_.order_storage()[security_id].active_orders().empty(),
        "Active orders must not be empty for cancellation query, security_id={}",
        security_id.value());
    const auto& active_orders = interface_.order_storage().at(security_id).active_orders();
    auto order_to_cancel_index = random_.rand_int<size_t>(0, active_orders.size() - 1);

    interface_.send_order_cancel_request(
        ClientID{0}, active_orders[order_to_cancel_index].client_order_id, security_id);
  }

  void generate_replace_order_request(SecurityID security_id) {
    CHECK(!interface_.order_storage()[security_id].active_orders().empty(),
        "Active orders must not be empty for replacement query, security_id={}",
        security_id.value());
    const auto& active_orders = interface_.order_storage().at(security_id).active_orders();
    auto order_to_replace_index = random_.rand_int<size_t>(0, active_orders.size() - 1);

    const Security& security = interface_.security_storage().get_security(security_id);

    auto resulting_price = random_.rand_decimal<Price>(security.low_price, security.high_price, security.tick_size);
    auto resulting_amount = random_.rand_decimal<Amount>(security.lot_size, security.max_amount, security.lot_size);

    auto direction = random_.rand_enum<Direction>();

    interface_.send_order_replace_request(ClientID{0},
        active_orders[order_to_replace_index].client_order_id,
        security_id,
        direction,
        resulting_price,
        resulting_amount);
  }

  GeneratorInterface& interface_;

  const uint32_t min_active_orders_by_security_;
  const uint32_t max_active_orders_by_security_;
  const uint32_t max_pending_transactions_;

  Random random_;
};

}  // namespace exchange_sim
