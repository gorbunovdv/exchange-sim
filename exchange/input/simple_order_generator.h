#pragma once

#include "base/logging/loggable_class.h"
#include "base/random.h"
#include "base/throughput.h"

#include "domain/security.h"

#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"

#include <boost/hof.hpp>
#include <glaze/glaze.hpp>

#include <random>

namespace exchange_sim {

template<typename GeneratorInterface>
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

  void process(const market_events::EventType& snapshot) {
    std::visit(boost::hof::match(
                   [this](const market_events::BookOrderEntry& snapshot) {
                     DEBUG(
                         "Received market data snapshot, type=BookOrderEntry"
                         " origin_time={} order_id={} security_id={}"
                         " direction={} price={} quantity={}",
                         snapshot.origin_time.ticks(),
                         snapshot.order_id.value(),
                         snapshot.security_id.value(),
                         format_as(snapshot.direction),
                         static_cast<double>(snapshot.price.value()),
                         static_cast<double>(snapshot.quantity.value()));
                   },
                   [this](const market_events::BookOrderChange& snapshot) {
                     DEBUG(
                         "Received market data snapshot, type=BookOrderChange"
                         " origin_time={} order_id={} security_id={} direction={} price={} new_quantity={}",
                         snapshot.origin_time.ticks(),
                         snapshot.order_id.value(),
                         snapshot.security_id.value(),
                         format_as(snapshot.direction),
                         static_cast<double>(snapshot.price.value()),
                         static_cast<double>(snapshot.new_quantity.value()));
                   },
                   [this](const market_events::BookOrderDeletion& snapshot) {
                     DEBUG(
                         "Received market data snapshot, type=BookOrderDeletion"
                         " origin_time={} order_id={} security_id={}",
                         snapshot.origin_time.ticks(),
                         snapshot.order_id.value(),
                         snapshot.security_id.value());
                   },
                   [this](const market_events::Trade& snapshot) {
                     DEBUG(
                         "Received market data snapshot, type=Trade"
                         " origin_time={} security_id={} price={} quantity={}"
                         " trade_id={} aggressor_order_id={} recipient_order_id={}",
                         snapshot.origin_time.ticks(),
                         snapshot.security_id.value(),
                         static_cast<double>(snapshot.price.value()),
                         static_cast<double>(snapshot.quantity.value()),
                         snapshot.trade_id.value(),
                         snapshot.aggressor_order_id.value(),
                         snapshot.recipient_order_id.value());
                   }),
        snapshot);
  }

  void process(const report::ReportType& report) {
    std::visit(
        boost::hof::match(
            [this](const report::OrderAccepted& report) {
              DEBUG(
                  "Received report, type=OrderAccepted"
                  " origin_time={} order_id={} client_id={} client_order_id={} security_id={} time_in_force={}"
                  " direction={} price={} quantity={}",
                  report.origin_time.ticks(),
                  report.order_id.value(),
                  report.client_id.value(),
                  report.client_order_id.value(),
                  report.security_id.value(),
                  format_as(report.time_in_force),
                  format_as(report.direction),
                  static_cast<double>(report.price.value()),
                  static_cast<double>(report.quantity.value()));
            },
            [this](const report::Trade& report) {
              DEBUG(
                  "Received report, type=Trade"
                  " origin_time={} security_id={} aggressor_direction={} price={} quantity={}"
                  " client_id={} client_order_id={} trade_id={} aggressor_order_id={} recipient_order_id={}",
                  report.origin_time.ticks(),
                  report.security_id.value(),
                  format_as(report.aggressor_direction),
                  static_cast<double>(report.price.value()),
                  static_cast<double>(report.quantity.value()),
                  report.client_id.value(),
                  report.client_order_id.value(),
                  report.trade_id.value(),
                  report.aggressor_order_id.value(),
                  report.recipient_order_id.value());
            },
            [this](const report::OrderBookEntry& report) {
              DEBUG(
                  "Received report, type=OrderBookEntry"
                  " origin_time={} order_id={} client_id={} client_order_id={} security_id={}"
                  " direction={} price={} quantity={}",
                  report.origin_time.ticks(),
                  report.order_id.value(),
                  report.client_id.value(),
                  report.client_order_id.value(),
                  report.security_id.value(),
                  format_as(report.direction),
                  static_cast<double>(report.price.value()),
                  static_cast<double>(report.quantity.value()));
            },
            [this](const report::OrderFilled& report) {
              DEBUG(
                  "Received report, type=OrderFilled"
                  " origin_time={} order_id={} client_id={} client_order_id={} security_id={}"
                  " direction={} price={} type={}",
                  report.origin_time.ticks(),
                  report.order_id.value(),
                  report.client_id.value(),
                  report.client_order_id.value(),
                  report.security_id.value(),
                  format_as(report.direction),
                  static_cast<double>(report.price.value()),
                  magic_enum::enum_name(report.type));
            },
            [this](const report::OrderExpired& report) {
              DEBUG(
                  "Received report, type=OrderExpired"
                  " origin_time={} order_id={} client_id={} client_order_id={} security_id={} reason={}",
                  report.origin_time.ticks(),
                  report.order_id.value(),
                  report.client_id.value(),
                  report.client_order_id.value(),
                  report.security_id.value(),
                  magic_enum::enum_name(report.reason));
            },
            [this](const report::OrderCancelled& report) {
              DEBUG(
                  "Received report, type=OrderCancelled"
                  " origin_time={} order_id={} client_id={} client_order_id={} client_cancellation_id={} "
                  "security_id={}",
                  report.origin_time.ticks(),
                  report.order_id.value(),
                  report.client_id.value(),
                  report.client_order_id.value(),
                  report.client_cancellation_id.value(),
                  report.security_id.value());
            },
            [this](const report::OrderRejected& report) {
              DEBUG(
                  "Received report, type=OrderRejected"
                  " origin_time={} security_id={} client_id={} client_order_id={} reason={}",
                  report.origin_time.ticks(),
                  report.security_id.value(),
                  report.client_id.value(),
                  report.client_order_id.value(),
                  magic_enum::enum_name(report.reason));
            },
            [this](const report::CancellationRejected& report) {
              DEBUG(
                  "Received report, type=CancellationRejected"
                  " origin_time={} client_id={} client_order_id={} client_cancellation_id={} security_id={} reason={}",
                  report.origin_time.ticks(),
                  report.client_id.value(),
                  report.client_order_id.value(),
                  report.client_cancellation_id.value(),
                  report.security_id.value(),
                  magic_enum::enum_name(report.reason));
            },
            [this](const report::ReplacementAccepted& report) {
              DEBUG(
                  "Received report, type=ReplacementAccepted"
                  " origin_time={} order_id={} new_order_id={} client_id={} client_order_id={} client_replacement_id={}"
                  " security_id={} direction={} price={} quantity={}",
                  report.origin_time.ticks(),
                  report.order_id.value(),
                  report.new_order_id.value(),
                  report.client_id.value(),
                  report.client_order_id.value(),
                  report.client_replacement_id.value(),
                  report.security_id.value(),
                  format_as(report.direction),
                  static_cast<double>(report.price.value()),
                  static_cast<double>(report.quantity.value()));
            },
            [this](const report::ReplacementRejected& report) {
              DEBUG(
                  "Received report, type=ReplacementRejected"
                  " origin_time={} client_id={} client_order_id={} client_replacement_id={} security_id={} reason={}",
                  report.origin_time.ticks(),
                  report.client_id.value(),
                  report.client_order_id.value(),
                  report.client_replacement_id.value(),
                  report.security_id.value(),
                  magic_enum::enum_name(report.reason));
            }),
        report);
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
    auto resulting_amount = random_.rand_decimal<Amount>(Amount{0}, security.max_amount, security.lot_size);

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
