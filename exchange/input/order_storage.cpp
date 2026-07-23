#include "order_storage.h"

#include "base/macros.h"
#include "base/one_of.h"

namespace exchange_sim {

void OrderStorage::handle(const request::OrderAddRequest& request) {
  CHECK(orders_.find({request.client_id, request.client_order_id}) == orders_.end(),
      "Order already exists, client_id={} client_order_id={}",
      request.client_id,
      request.client_order_id);
  pending_transactions_count_++;
  orders_.emplace(std::pair{request.client_id, request.client_order_id},
      Order{
          .client_id = request.client_id,
          .client_order_id = request.client_order_id,
          .security_id = request.security_id,
          .state = Order::State::SENT,
          .filled = false,
      });
}

void OrderStorage::handle(const request::OrderCancelRequest& request) {
  auto order = find_order(request.client_id, request.client_order_id);
  CHECK(order->second.state == Order::State::ACTIVE,
      "Order must be active to be cancelled, client_id={} client_order_id={}",
      request.client_id,
      request.client_order_id);
  ++pending_transactions_count_;
  active_orders_.erase({order->second.client_id, order->second.client_order_id});
  order->second.state = Order::State::CANCEL_SENT;
}

void OrderStorage::handle(const request::OrderReplaceRequest& request) {
  auto order = find_order(request.client_id, request.client_order_id);
  CHECK(order->second.state == Order::State::ACTIVE,
      "Order must be active to be replaced, client_id={} client_order_id={}",
      request.client_id,
      request.client_order_id);
  ++pending_transactions_count_;
  active_orders_.erase({order->second.client_id, order->second.client_order_id});
  order->second.state = Order::State::REPLACE_SENT;
}

void OrderStorage::handle(const report::OrderAccepted& report) {
  auto order = find_order(report.client_id, report.client_order_id);
  CHECK(order->second.state == Order::State::SENT,
      "Order must be sent to be accepted, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  order->second.state = Order::State::ACCEPTED;
}

void OrderStorage::handle(const report::OrderRejected& report) {
  auto order = find_order(report.client_id, report.client_order_id);
  CHECK(order->second.state == Order::State::SENT,
      "Order must be sent to be rejected, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  pending_transactions_count_--;
  orders_.erase(order);
}

void OrderStorage::handle(const report::OrderBookEntry& report) {
  auto order = find_order(report.client_id, report.client_order_id);
  CHECK(order->second.state == Order::State::ACCEPTED,
      "Order must be accepted to become active, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  active_orders_.emplace({order->second.client_id, order->second.client_order_id}, order->second);
  pending_transactions_count_--;
  order->second.state = Order::State::ACTIVE;
}

void OrderStorage::handle(const report::OrderExpired& report) {
  auto order = find_order(report.client_id, report.client_order_id);
  CHECK(order->second.state == Order::State::ACCEPTED,
      "Order must be accepted to expire, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  pending_transactions_count_--;
  orders_.erase(order);
}

void OrderStorage::handle(const report::OrderFilled& report) {
  auto order = find_order(report.client_id, report.client_order_id);
  CHECK(equal_to_one_of(order->second.state,
            Order::State::ACCEPTED,
            Order::State::ACTIVE,
            Order::State::CANCEL_SENT,
            Order::State::REPLACE_SENT),
      "Order must be accepted, active, cancel_sent or replace_sent to be filled, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  if (order->second.state == Order::State::ACCEPTED) {
    pending_transactions_count_--;
  } else if (order->second.state == Order::State::ACTIVE) {
    active_orders_.erase({order->second.client_id, order->second.client_order_id});
  }
  order->second.filled = true;
  if (!equal_to_one_of(order->second.state, Order::State::CANCEL_SENT, Order::State::REPLACE_SENT)) {
    orders_.erase(order);
  }
}

void OrderStorage::handle(const report::OrderCancelled& report) {
  auto order = find_order(report.client_id, report.client_order_id);
  CHECK(order->second.state == Order::State::CANCEL_SENT,
      "Cancel must be sent for order to be cancelled, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  pending_transactions_count_--;
  orders_.erase(order);
}

void OrderStorage::handle(const report::CancellationRejected& report) {
  auto order = find_order(report.client_id, report.client_order_id);
  CHECK(order->second.state == Order::State::CANCEL_SENT,
      "Cancel must be sent for cancellation to be rejected, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  pending_transactions_count_--;
  if (!order->second.filled) {
    active_orders_.emplace({order->second.client_id, order->second.client_order_id}, order->second);
    order->second.state = Order::State::ACTIVE;
  } else {
    orders_.erase(order);
  }
}

void OrderStorage::handle(const report::ReplacementAccepted& report) {
  auto order = find_order(report.client_id, report.client_order_id);
  CHECK(order->second.state == Order::State::REPLACE_SENT,
      "Replace must be sent for replace to be accepted, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  orders_.erase(order);
  orders_.emplace(std::pair{report.client_id, report.client_replacement_id},
      Order{
          .client_id = report.client_id,
          .client_order_id = report.client_replacement_id,
          .security_id = report.security_id,
          .state = Order::State::ACCEPTED,
          .filled = false,
      });
}

void OrderStorage::handle(const report::ReplacementRejected& report) {
  auto order = find_order(report.client_id, report.client_order_id);
  CHECK(order->second.state == Order::State::REPLACE_SENT,
      "Replace must be sent for replace to be rejected, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  pending_transactions_count_--;
  if (!order->second.filled) {
    active_orders_.emplace({order->second.client_id, order->second.client_order_id}, order->second);
    order->second.state = Order::State::ACTIVE;
  } else {
    orders_.erase(order);
  }
}

OrderStorage::OrderContainer::iterator OrderStorage::find_order(ClientID client_id, ClientOrderID client_order_id) {
  auto it = orders_.find({client_id, client_order_id});
  CHECK(it != orders_.end(), "Order not found, client_id={} client_order_id={}", client_id, client_order_id);
  return it;
}

}  // namespace exchange_sim
