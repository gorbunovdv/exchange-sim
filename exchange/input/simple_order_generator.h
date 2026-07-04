#pragma once

#include "base/counter.h"
#include "base/logging/loggable_class.h"

#include "domain/security.h"
#include "domain/security_storage.h"

#include "exchange/domain/market_data_snapshots.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"
#include "exchange/input/order_storage.h"

#include <boost/hof.hpp>
#include <glaze/glaze.hpp>

#include <random>

namespace exchange_sim {

template<request::Sink SinkType>
class SimpleOrderGenerator : public LoggableClass {
public:
  explicit SimpleOrderGenerator(
      SecurityStorage& security_storage, const glz::generic& config, SinkType& matching_engine_sink):
      LoggableClass("SimpleOrderGenerator"),
      matching_engine_sink_(matching_engine_sink),
      min_active_orders_by_security_(config["min_active_orders_by_security"].as<std::uint32_t>()),
      max_active_orders_by_security_(config["max_active_orders_by_security"].as<std::uint32_t>()),
      security_storage_(security_storage),
      active_orders_(security_storage_.size()),
      generator_(config["seed"].as<std::uint32_t>()),
      security_id_distribution_(0, security_storage_.size() - 1) {
    CHECK(min_active_orders_by_security_ <= max_active_orders_by_security_,
        "Min active orders must be less than max, min_active_orders_by_security={} max_active_orders_by_security={}",
        min_active_orders_by_security_,
        max_active_orders_by_security_);
    CHECK(min_active_orders_by_security_ > 0,
        "Min active orders must be greater than 0, min_active_orders_by_security={}",
        min_active_orders_by_security_);
  }

  void run() {
    while (!asked_to_stop()) {
      auto order = generate_event();
      if (std::holds_alternative<request::OrderAddRequest>(order)) {
        auto& request = std::get<request::OrderAddRequest>(order);
        active_orders_[request.security_id].handle_order_sent(request);
      }
      if (std::holds_alternative<request::OrderCancelRequest>(order)) {
        auto& request = std::get<request::OrderCancelRequest>(order);
        active_orders_[request.security_id].handle_cancel_sent(request);
      }
      if (std::holds_alternative<request::OrderReplaceRequest>(order)) {
        auto& request = std::get<request::OrderReplaceRequest>(order);
        active_orders_[request.security_id].handle_order_replace_sent(request);
      }
      matching_engine_sink_.push(std::move(order));
    }
  }

  bool asked_to_stop() const { return false; }

  void process(market_data_snapshot::SnapshotType&& snapshot) {
    std::visit(boost::hof::match(
                   [this](market_data_snapshot::BookOrderEntry&& snapshot) {
                     INFO(
                         "Received market data snapshot, type=BookOrderEntry"
                         " order_id={} security_id={}"
                         " direction={} price={} quantity={}",
                         snapshot.order_id,
                         snapshot.security_id.value(),
                         format_as(snapshot.direction),
                         static_cast<double>(snapshot.price),
                         static_cast<double>(snapshot.quantity));
                   },
                   [this](market_data_snapshot::BookOrderChange&& snapshot) {
                     INFO(
                         "Received market data snapshot, type=BookOrderChange"
                         " order_id={} security_id={} direction={} price={} new_quantity={}",
                         snapshot.order_id,
                         snapshot.security_id.value(),
                         format_as(snapshot.direction),
                         static_cast<double>(snapshot.price),
                         static_cast<double>(snapshot.new_quantity));
                   },
                   [this](market_data_snapshot::BookOrderDeletion&& snapshot) {
                     INFO(
                         "Received market data snapshot, type=BookOrderDeletion"
                         " order_id={} security_id={}",
                         snapshot.order_id,
                         snapshot.security_id.value());
                   },
                   [this](market_data_snapshot::Trade&& snapshot) {
                     INFO(
                         "Received market data snapshot, type=Trade"
                         " security_id={} price={} quantity={}"
                         " trade_id={} aggressor_order_id={} recipient_order_id={}",
                         snapshot.security_id.value(),
                         static_cast<double>(snapshot.price),
                         static_cast<double>(snapshot.quantity),
                         snapshot.trade_id,
                         snapshot.aggressor_order_id,
                         snapshot.recipient_order_id);
                   }),
        std::move(snapshot));
  }

  void process(report::ReportType report) {
    std::visit(
        boost::hof::match(
            [this](report::OrderAccepted&& report) {
              active_orders_[report.security_id].handle_order_accepted(report);
              INFO(
                  "Received report, type=OrderAccepted"
                  " order_id={} client_id={} client_order_id={} security_id={} time_in_force={}"
                  " direction={} price={} quantity={}",
                  report.order_id,
                  report.client_id,
                  report.client_order_id,
                  report.security_id.value(),
                  format_as(report.time_in_force),
                  format_as(report.direction),
                  static_cast<double>(report.price),
                  static_cast<double>(report.quantity));
            },
            [this](report::Trade&& report) {
              INFO(
                  "Received report, type=Trade"
                  " security_id={} aggressor_direction={} price={} quantity={}"
                  " client_id={} client_order_id={} trade_id={} aggressor_order_id={} recipient_order_id={}",
                  report.security_id.value(),
                  format_as(report.aggressor_direction),
                  static_cast<double>(report.price),
                  static_cast<double>(report.quantity),
                  report.client_id,
                  report.client_order_id,
                  report.trade_id,
                  report.aggressor_order_id,
                  report.recipient_order_id);
            },
            [this](report::OrderBookEntry&& report) {
              active_orders_[report.security_id].handle_order_book_entry(report);
              INFO(
                  "Received report, type=OrderBookEntry"
                  " order_id={} client_id={} client_order_id={} security_id={} direction={} price={} quantity={}",
                  report.order_id,
                  report.client_id,
                  report.client_order_id,
                  report.security_id.value(),
                  format_as(report.direction),
                  static_cast<double>(report.price),
                  static_cast<double>(report.quantity));
            },
            [this](report::OrderFilled&& report) {
              active_orders_[report.security_id].handle_order_filled(report);
              INFO(
                  "Received report, type=OrderFilled"
                  " order_id={} client_id={} client_order_id={} security_id={} direction={} price={} type={}",
                  report.order_id,
                  report.client_id,
                  report.client_order_id,
                  report.security_id.value(),
                  format_as(report.direction),
                  static_cast<double>(report.price),
                  magic_enum::enum_name(report.type));
            },
            [this](report::OrderExpired&& report) {
              active_orders_[report.security_id].handle_order_expired(report);
              INFO(
                  "Received report, type=OrderExpired"
                  " order_id={} client_id={} client_order_id={} security_id={} reason={}",
                  report.order_id,
                  report.client_id,
                  report.client_order_id,
                  report.security_id.value(),
                  magic_enum::enum_name(report.reason));
            },
            [this](report::OrderCancelled&& report) {
              active_orders_[report.security_id].handle_order_canceled(report);
              INFO(
                  "Received report, type=OrderCancelled"
                  " order_id={} client_id={} client_order_id={} client_cancellation_id={} security_id={}",
                  report.order_id,
                  report.client_id,
                  report.client_order_id,
                  report.client_cancellation_id,
                  report.security_id.value());
            },
            [this](report::OrderRejected&& report) {
              active_orders_[report.security_id].handle_order_rejected(report);
              INFO(
                  "Received report, type=OrderRejected"
                  " security_id={} client_id={} client_order_id={} reason={}",
                  report.security_id.value(),
                  report.client_id,
                  report.client_order_id,
                  magic_enum::enum_name(report.reason));
            },
            [this](report::CancellationRejected&& report) {
              active_orders_[report.security_id].handle_cancel_rejected(report);
              INFO(
                  "Received report, type=CancellationRejected"
                  " client_id={} client_order_id={} client_cancellation_id={} security_id={} reason={}",
                  report.client_id,
                  report.client_order_id,
                  report.client_cancellation_id,
                  report.security_id.value(),
                  magic_enum::enum_name(report.reason));
            },
            [this](report::ReplacementAccepted&& report) {
              active_orders_[report.security_id].handle_replace_accepted(report);
              INFO(
                  "Received report, type=ReplacementAccepted"
                  " order_id={} new_order_id={} client_id={} client_order_id={} client_replacement_id={}"
                  " security_id={} direction={} price={} quantity={}",
                  report.order_id,
                  report.new_order_id,
                  report.client_id,
                  report.client_order_id,
                  report.client_replacement_id,
                  report.security_id.value(),
                  format_as(report.direction),
                  static_cast<double>(report.price),
                  static_cast<double>(report.quantity));
            },
            [this](report::ReplacementRejected&& report) {
              active_orders_[report.security_id].handle_replace_rejected(report);
              INFO(
                  "Received report, type=ReplacementRejected"
                  " client_id={} client_order_id={} client_replacement_id={} security_id={} reason={}",
                  report.client_id,
                  report.client_order_id,
                  report.client_replacement_id,
                  report.security_id.value(),
                  magic_enum::enum_name(report.reason));
            }),
        std::move(report));
  }

private:
  request::RequestType generate_event() {
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
      return generate_new_order_request(SecurityID{security_id_distribution_(generator_)});
    }
    if (type == 1) {
      return generate_cancel_order_request(SecurityID{security_id_distribution_(generator_)});
    }
    if (type == 2 && std::ranges::any_of(active_orders_, [](auto& pair) { return pair.size() == 0; })) {
      return generate_new_order_request(SecurityID{security_id_distribution_(generator_)});
    }
    return generate_replace_order_request(SecurityID{security_id_distribution_(generator_)});
  }

  request::OrderAddRequest generate_new_order_request(SecurityID security_id) {
    const Security& security = security_storage_.get_security(security_id);
    Price price_interval = security.high_price - security.low_price;
    int64_t price_max_multiplied_value = static_cast<uint64_t>(price_interval / security.tick_size);

    Price resulting_price =
        std::uniform_int_distribution<uint64_t>(0, price_max_multiplied_value)(generator_) * security.tick_size +
        security.low_price;
    Amount resulting_amount = std::uniform_int_distribution<uint64_t>(
                                  1, static_cast<uint64_t>(security.max_amount / security.lot_size))(generator_) *
        security.lot_size;
    OrderTimeInForce time_in_force = random_enum_value<OrderTimeInForce>(generator_);
    Direction direction = random_enum_value<Direction>(generator_);

    return request::OrderAddRequest{
        .client_id = 0,
        .client_order_id = generate_order_id(),
        .security_id = security_id,
        .time_in_force = time_in_force,
        .order_type = OrderType::LIMIT,
        .direction = direction,
        .price = resulting_price,
        .quantity = resulting_amount,
    };
  }

  request::OrderCancelRequest generate_cancel_order_request(SecurityID security_id) {
    CHECK(!active_orders_[security_id].empty(),
        "Active orders must not be empty for cancellation query, security_id={}",
        security_id.value());
    const auto& active_orders = active_orders_.at(security_id);
    size_t order_to_cancel_index = std::uniform_int_distribution<size_t>(0, active_orders.size() - 1)(generator_);

    return request::OrderCancelRequest{
        .client_id = 0,
        .client_order_id = active_orders[order_to_cancel_index].client_order_id,
        .client_cancellation_id = generate_order_id(),
        .security_id = security_id,
    };
  }

  request::OrderReplaceRequest generate_replace_order_request(SecurityID security_id) {
    CHECK(!active_orders_[security_id].empty(),
        "Active orders must not be empty for replacement query, security_id={}",
        security_id.value());
    const auto& active_orders = active_orders_.at(security_id);
    size_t order_to_replace_index = std::uniform_int_distribution<size_t>(0, active_orders.size() - 1)(generator_);

    const Security& security = security_storage_.get_security(security_id);
    Price price_interval = security.high_price - security.low_price;
    int64_t price_max_multiplied_value = static_cast<uint64_t>(price_interval / security.tick_size);

    Price resulting_price =
        std::uniform_int_distribution<uint64_t>(0, price_max_multiplied_value)(generator_) * security.tick_size +
        security.low_price;
    Amount resulting_amount = std::uniform_int_distribution<uint64_t>(
                                  1, static_cast<uint64_t>(security.max_amount / security.lot_size))(generator_) *
        security.lot_size;

    Direction direction = random_enum_value<Direction>(generator_);

    return request::OrderReplaceRequest{
        .client_id = 0,
        .client_order_id = active_orders[order_to_replace_index].client_order_id,
        .client_replacement_id = generate_order_id(),
        .security_id = security_id,
        .direction = direction,
        .price = resulting_price,
        .quantity = resulting_amount,
    };
  }

  template<typename Enum>
  static Enum random_enum_value(std::mt19937& generator) {
    static_assert(std::is_enum_v<Enum>);
    static_assert(magic_enum::enum_count<Enum>() > 0);

    static constexpr auto values = magic_enum::enum_values<Enum>();
    return values[std::uniform_int_distribution<uint32_t>(0, magic_enum::enum_count<Enum>() - 1)(generator)];
  }

  ClientOrderID generate_order_id() { return current_client_order_id_.inc(); }

  struct ActiveOrder {
    ClientOrderID order_id;
  };

  SinkType& matching_engine_sink_;

  const uint32_t min_active_orders_by_security_;
  const uint32_t max_active_orders_by_security_;

  SecurityStorage& security_storage_;
  StrongTypeIndexedVector<SecurityID, OrderStorage> active_orders_;

  std::mt19937 generator_;
  std::uniform_int_distribution<std::uint32_t> type_distribution_{0, 2};
  std::uniform_int_distribution<SecurityID::UnderlyingType> security_id_distribution_;

  Counter<ClientOrderID> current_client_order_id_;
};

}  // namespace exchange_sim