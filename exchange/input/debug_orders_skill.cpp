#include "debug_orders_skill.h"

namespace exchange_sim::generator_interface_skills {


void DebugOrders::handle_request(const request::OrderAddRequest& request) {
  DEBUG(
      "Sent request, type=OrderAddRequest"
      " origin_time={} client_id={} client_order_id={} security_id={}"
      " time_in_force={} order_type={} direction={} price={} quantity={}",
      request.origin_time.ticks(),
      request.client_id.value(),
      request.client_order_id.value(),
      request.security_id.value(),
      format_as(request.time_in_force),
      format_as(request.order_type),
      format_as(request.direction),
      static_cast<double>(request.price.value()),
      static_cast<double>(request.quantity.value()));
}
void DebugOrders::handle_request(const request::OrderReplaceRequest& request) {
  DEBUG(
      "Sent request, type=OrderReplaceRequest"
      " origin_time={} client_id={} client_order_id={} client_replacement_id={} security_id={}"
      " direction={} price={} quantity={}",
      request.origin_time.ticks(),
      request.client_id.value(),
      request.client_order_id.value(),
      request.client_replacement_id.value(),
      request.security_id.value(),
      format_as(request.direction),
      static_cast<double>(request.price.value()),
      static_cast<double>(request.quantity.value()));
}
void DebugOrders::handle_request(const request::OrderCancelRequest& request) {
  DEBUG(
      "Sent request, type=OrderCancelRequest"
      " origin_time={} client_id={} client_order_id={} client_cancellation_id={} security_id={}",
      request.origin_time.ticks(),
      request.client_id.value(),
      request.client_order_id.value(),
      request.client_cancellation_id.value(),
      request.security_id.value());
}
void DebugOrders::handle_market_event(const market_events::BookOrderEntry& snapshot) {
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
}
void DebugOrders::handle_market_event(const market_events::BookOrderChange& snapshot) {
  DEBUG(
      "Received market data snapshot, type=BookOrderChange"
      " origin_time={} order_id={} security_id={} direction={} price={} new_quantity={}",
      snapshot.origin_time.ticks(),
      snapshot.order_id.value(),
      snapshot.security_id.value(),
      format_as(snapshot.direction),
      static_cast<double>(snapshot.price.value()),
      static_cast<double>(snapshot.new_quantity.value()));
}
void DebugOrders::handle_market_event(const market_events::BookOrderDeletion& snapshot) {
  DEBUG(
      "Received market data snapshot, type=BookOrderDeletion"
      " origin_time={} order_id={} security_id={}",
      snapshot.origin_time.ticks(),
      snapshot.order_id.value(),
      snapshot.security_id.value());
}
void DebugOrders::handle_market_event(const market_events::Trade& snapshot) {
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
}
void DebugOrders::handle_report(const report::OrderAccepted& report) {
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
}
void DebugOrders::handle_report(const report::Trade& report) {
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
}
void DebugOrders::handle_report(const report::OrderBookEntry& report) {
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
}
void DebugOrders::handle_report(const report::OrderFilled& report) {
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
}
void DebugOrders::handle_report(const report::OrderExpired& report) {
  DEBUG(
      "Received report, type=OrderExpired"
      " origin_time={} order_id={} client_id={} client_order_id={} security_id={} reason={}",
      report.origin_time.ticks(),
      report.order_id.value(),
      report.client_id.value(),
      report.client_order_id.value(),
      report.security_id.value(),
      magic_enum::enum_name(report.reason));
}
void DebugOrders::handle_report(const report::OrderCancelled& report) {
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
}
void DebugOrders::handle_report(const report::OrderRejected& report) {
  DEBUG(
      "Received report, type=OrderRejected"
      " origin_time={} security_id={} client_id={} client_order_id={} reason={}",
      report.origin_time.ticks(),
      report.security_id.value(),
      report.client_id.value(),
      report.client_order_id.value(),
      magic_enum::enum_name(report.reason));
}
void DebugOrders::handle_report(const report::CancellationRejected& report) {
  DEBUG(
      "Received report, type=CancellationRejected"
      " origin_time={} client_id={} client_order_id={} client_cancellation_id={} security_id={} reason={}",
      report.origin_time.ticks(),
      report.client_id.value(),
      report.client_order_id.value(),
      report.client_cancellation_id.value(),
      report.security_id.value(),
      magic_enum::enum_name(report.reason));
}
void DebugOrders::handle_report(const report::ReplacementAccepted& report) {
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
}
void DebugOrders::handle_report(const report::ReplacementRejected& report) {
  DEBUG(
      "Received report, type=ReplacementRejected"
      " origin_time={} client_id={} client_order_id={} client_replacement_id={} security_id={} reason={}",
      report.origin_time.ticks(),
      report.client_id.value(),
      report.client_order_id.value(),
      report.client_replacement_id.value(),
      report.security_id.value(),
      magic_enum::enum_name(report.reason));
}

}  // namespace exchange_sim::generator_interface_skills