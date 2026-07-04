#pragma once
#include "base/logging/logging_backend.h"

#include "domain/types.h"

#include "exchange/domain/types.h"

namespace exchange_sim {

struct Order {
  ClientID client_id;
  ClientOrderID client_order_id;
  SecurityID security_id;

  enum class State : std::uint8_t { SENT, ACCEPTED, ACTIVE, CANCEL_SENT, REPLACE_SENT };

  State state;
};

class OrderStorage {
public:
  OrderStorage() = default;

  void handle_order_sent(const request::OrderAddRequest& request) {
    CHECK(find_order_or_end(request.client_id, request.client_order_id) == orders_.end(),
        "Order already exists, client_id={} client_order_id={}",
        request.client_id,
        request.client_order_id);
    orders_.push_back(Order{
        .client_id = request.client_id,
        .client_order_id = request.client_order_id,
        .security_id = request.security_id,
        .state = Order::State::SENT,
    });
  }

  void handle_cancel_sent(const request::OrderCancelRequest& request) {
    CHECK(std::exchange(find_order(request.client_id, request.client_order_id)->state, Order::State::CANCEL_SENT) ==
            Order::State::ACTIVE,
        "Order must be active to be cancelled, client_id={} client_order_id={}",
        request.client_id,
        request.client_order_id);
  }

  void handle_replace_sent(const request::OrderReplaceRequest& request) {
    CHECK(std::exchange(find_order(request.client_id, request.client_order_id)->state, Order::State::REPLACE_SENT) ==
            Order::State::ACTIVE,
        "Order must be active to be replaced, client_id={} client_order_id={}",
        request.client_id,
        request.client_order_id);
  }

  std::vector<Order>::iterator find_order_or_end(ClientID client_id, ClientOrderID client_order_id) {
    return std::find_if(orders_.begin(), orders_.end(), [&](const Order& order) {
      return order.client_id == client_id && order.client_order_id == client_order_id;
    });
  }

  std::vector<Order>::iterator find_order(ClientID client_id, ClientOrderID client_order_id) {
    auto it = find_order_or_end(client_id, client_order_id);
    CHECK(it != orders_.end(), "Order not found, client_id={} client_order_id={}", client_id, client_order_id);
    return it;
  }

  void handle_order_accepted(const report::OrderAccepted& report) {
    find_order(report.client_id, report.client_order_id)->state = Order::State::ACCEPTED;
  }

  void handle_order_rejected(const report::OrderRejected& report) {
    orders_.erase(find_order(report.client_id, report.client_order_id));
  }

  void handle_order_book_entry(const report::OrderBookEntry& report) {
    CHECK(std::exchange(find_order(report.client_id, report.client_order_id)->state, Order::State::ACTIVE) ==
            Order::State::ACCEPTED,
        "Order must be accepted to become active, client_id={} client_order_id={}",
        report.client_id,
        report.client_order_id);
  }

  void handle_order_expired(const report::OrderExpired& report) {
    orders_.erase(find_order(report.client_id, report.client_order_id));
  }

  void handle_order_filled(const report::OrderFilled& report) {
    orders_.erase(find_order(report.client_id, report.client_order_id));
  }

  void handle_order_canceled(const report::OrderCancelled& report) {
    orders_.erase(find_order(report.client_id, report.client_order_id));
  }

  void handle_cancel_rejected(const report::CancellationRejected& report) {
    CHECK(std::exchange(find_order(report.client_id, report.client_order_id)->state, Order::State::ACTIVE) ==
            Order::State::CANCEL_SENT,
        "Cancel must be sent for cancellation to be rejected, client_id={} client_order_id={}",
        report.client_id,
        report.client_order_id);
  }

  void handle_replace_accepted(const report::ReplacementAccepted& report) {
    orders_.erase(find_order(report.client_id, report.client_order_id));
    orders_.push_back(Order{
        .client_id = report.client_id,
        .client_order_id = report.client_replacement_id,
        .security_id = report.security_id,
        .state = Order::State::ACCEPTED,
    });
  }

  void handle_replace_rejected(const report::ReplacementRejected& report) {
    CHECK(std::exchange(find_order(report.client_id, report.client_order_id)->state, Order::State::ACTIVE) ==
            Order::State::REPLACE_SENT,
        "Replace must be sent for replace to be rejected, client_id={} client_order_id={}",
        report.client_id,
        report.client_order_id);
  }

  bool empty() const { return size() == 0; }

  size_t size() const {
    return std::ranges::count_if(orders_, [](const auto& order) { return order.state == Order::State::ACTIVE; });
  }

  size_t pending_transactions() const {
    return std::ranges::count_if(orders_, [](const auto& order) {
      return order.state == Order::State::SENT || order.state == Order::State::ACCEPTED ||
          order.state == Order::State::CANCEL_SENT || order.state == Order::State::REPLACE_SENT;
    });
  }

  const Order& operator[](size_t index) const {
    auto active_orders =
        orders_ | std::views::filter([](const Order& order) { return order.state == Order::State::ACTIVE; });
    auto it = std::ranges::next(active_orders.begin(), static_cast<std::ptrdiff_t>(index), active_orders.end());
    CHECK(it != active_orders.end(), "Index out of range");
    return *it;
  }

private:
  std::vector<Order> orders_;
};

}  // namespace exchange_sim