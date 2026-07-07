#include "order_storage.h"

namespace exchange_sim {

void OrderStorage::handle(const request::OrderAddRequest& request) {
  CHECK(find_order_or_end(request.client_id, request.client_order_id) == orders_.end(),
      "Order already exists, client_id={} client_order_id={}",
      request.client_id,
      request.client_order_id);
  orders_.push_front(Order{
      .client_id = request.client_id,
      .client_order_id = request.client_order_id,
      .security_id = request.security_id,
      .state = Order::State::SENT,
  });
  order_map_.emplace(std::pair{request.client_id, request.client_order_id}, orders_.begin());
}

void OrderStorage::handle(const request::OrderCancelRequest& request) {
  CHECK(std::exchange(find_order(request.client_id, request.client_order_id)->state, Order::State::CANCEL_SENT) ==
          Order::State::ACTIVE,
      "Order must be active to be cancelled, client_id={} client_order_id={}",
      request.client_id,
      request.client_order_id);
}

void OrderStorage::handle(const request::OrderReplaceRequest& request) {
  CHECK(std::exchange(find_order(request.client_id, request.client_order_id)->state, Order::State::REPLACE_SENT) ==
          Order::State::ACTIVE,
      "Order must be active to be replaced, client_id={} client_order_id={}",
      request.client_id,
      request.client_order_id);
}

void OrderStorage::handle(const report::OrderAccepted& report) {
  CHECK(std::exchange(find_order(report.client_id, report.client_order_id)->state, Order::State::ACCEPTED) ==
          Order::State::SENT,
      "Order must be sent to be accepted, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
}

void OrderStorage::handle(const report::OrderRejected& report) {
  auto it = find_order(report.client_id, report.client_order_id);
  CHECK(it->state == Order::State::SENT,
      "Order must be sent to be rejected, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  orders_.erase(find_order(report.client_id, report.client_order_id));
  order_map_.erase({report.client_id, report.client_order_id});
}

void OrderStorage::handle(const report::OrderBookEntry& report) {
  CHECK(std::exchange(find_order(report.client_id, report.client_order_id)->state, Order::State::ACTIVE) ==
          Order::State::ACCEPTED,
      "Order must be accepted to become active, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
}

void OrderStorage::handle(const report::OrderExpired& report) {
  auto it = find_order(report.client_id, report.client_order_id);
  CHECK(it->state == Order::State::ACCEPTED,
      "Order must be accepted to expire, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  orders_.erase(it);
  order_map_.erase({report.client_id, report.client_order_id});
}

void OrderStorage::handle(const report::OrderFilled& report) {
  auto it = find_order(report.client_id, report.client_order_id);
  CHECK(it->state == Order::State::ACCEPTED || it->state == Order::State::ACTIVE,
      "Order must be accepted or active to be filled, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  orders_.erase(it);
  order_map_.erase({report.client_id, report.client_order_id});
}

void OrderStorage::handle(const report::OrderCancelled& report) {
  auto it = find_order(report.client_id, report.client_order_id);
  CHECK(it->state == Order::State::CANCEL_SENT,
      "Cancel must be sent for order to be cancelled, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  orders_.erase(it);
  order_map_.erase({report.client_id, report.client_order_id});
}

void OrderStorage::handle(const report::CancellationRejected& report) {
  CHECK(std::exchange(find_order(report.client_id, report.client_order_id)->state, Order::State::ACTIVE) ==
          Order::State::CANCEL_SENT,
      "Cancel must be sent for cancellation to be rejected, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
}

void OrderStorage::handle(const report::ReplacementAccepted& report) {
  auto it = find_order(report.client_id, report.client_order_id);
  CHECK(it->state == Order::State::REPLACE_SENT,
      "Replace must be sent for replace to be accepted, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
  orders_.erase(it);
  order_map_.erase({report.client_id, report.client_order_id});
  orders_.push_front(Order{
      .client_id = report.client_id,
      .client_order_id = report.client_replacement_id,
      .security_id = report.security_id,
      .state = Order::State::ACCEPTED,
  });
  order_map_.emplace(std::pair{report.client_id, report.client_replacement_id}, orders_.begin());
}

void OrderStorage::handle(const report::ReplacementRejected& report) {
  CHECK(std::exchange(find_order(report.client_id, report.client_order_id)->state, Order::State::ACTIVE) ==
          Order::State::REPLACE_SENT,
      "Replace must be sent for replace to be rejected, client_id={} client_order_id={}",
      report.client_id,
      report.client_order_id);
}

const Order& OrderStorage::operator[](size_t index) const {
  auto active_orders =
      orders_ | std::views::filter([](const Order& order) { return order.state == Order::State::ACTIVE; });
  auto it = std::ranges::next(active_orders.begin(), static_cast<std::ptrdiff_t>(index), active_orders.end());
  CHECK(it != active_orders.end(), "Index out of range");
  return *it;
}

std::list<Order>::iterator OrderStorage::find_order_or_end(ClientID client_id, ClientOrderID client_order_id) {
  if (auto it = order_map_.find({client_id, client_order_id}); it != order_map_.end()) {
    return it->second;
  }
  return orders_.end();
}

std::list<Order>::iterator OrderStorage::find_order(ClientID client_id, ClientOrderID client_order_id) {
  auto it = find_order_or_end(client_id, client_order_id);
  CHECK(it != orders_.end(), "Order not found, client_id={} client_order_id={}", client_id, client_order_id);
  return it;
}

}  // namespace exchange_sim
