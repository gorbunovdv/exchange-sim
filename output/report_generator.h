#pragma once

#include "base/macros.h"
#include "base/logging/loggable_class.h"
#include "domain/reports.h"

#include <boost/hof/match.hpp>
#include <glaze/json/generic_fwd.hpp>

namespace exchange_sim {

class ReportGenerator : public LoggableClass {
public:
  explicit ReportGenerator(const glz::generic&): LoggableClass("report_generator") {}

  void process(report::ReportType report) {
    std::visit(
        boost::hof::match(
            [this](report::OrderAccepted report) {
              INFO(
                  "Received report, type=OrderAccepted"
                  " order_id={} client_id={} client_order_id={} security_id={} time_in_force={}"
                  " direction={} price={} quantity={}",
                  report.order_id,
                  report.client_id,
                  report.client_order_id,
                  report.security_id,
                  format_as(report.time_in_force),
                  format_as(report.direction),
                  static_cast<double>(report.price),
                  static_cast<double>(report.quantity));
            },
            [this](report::Trade report) {
              INFO(
                  "Received report, type=Trade"
                  " security_id={} aggressor_direction={} price={} quantity={}"
                  " client_id={} client_order_id={} trade_id={} aggressor_order_id={} recipient_order_id={}",
                  report.security_id,
                  format_as(report.aggressor_direction),
                  static_cast<double>(report.price),
                  static_cast<double>(report.quantity),
                  report.client_id,
                  report.client_order_id,
                  report.trade_id,
                  report.aggressor_order_id,
                  report.recipient_order_id);
            },
            [this](report::OrderBookEntry report) {
              INFO(
                  "Received report, type=OrderBookEntry"
                  " order_id={} client_id={} client_order_id={} security_id={} direction={} price={} quantity={}",
                  report.order_id,
                  report.client_id,
                  report.client_order_id,
                  report.security_id,
                  format_as(report.direction),
                  static_cast<double>(report.price),
                  static_cast<double>(report.quantity));
            },
            [this](report::OrderFilled report) {
              INFO(
                  "Received report, type=OrderFilled"
                  " order_id={} client_id={} client_order_id={} security_id={}"
                  " direction={} price={} remaining_quantity={} type={} fully_filled={}",
                  report.order_id,
                  report.client_id,
                  report.client_order_id,
                  report.security_id,
                  format_as(report.direction),
                  static_cast<double>(report.price),
                  static_cast<double>(report.remaining_quantity),
                  magic_enum::enum_name(report.type),
                  report.fully_filled);
            },
            [this](report::OrderExpired report) {
              INFO(
                  "Received report, type=OrderExpired"
                  " order_id={} client_id={} client_order_id={} security_id={} reason={}",
                  report.order_id,
                  report.client_id,
                  report.client_order_id,
                  report.security_id,
                  magic_enum::enum_name(report.reason));
            },
            [this](report::OrderCancelled report) {
              INFO(
                  "Received report, type=OrderCancelled"
                  " order_id={} client_id={} client_order_id={}"
                  " client_cancellation_id={} security_id={}",
                  report.order_id,
                  report.client_id,
                  report.client_order_id,
                  report.client_cancellation_id,
                  report.security_id);
            },
            [this](report::OrderRejected report) {
              INFO(
                  "Received report, type=OrderRejected"
                  " security_id={} client_id={} client_order_id={} reason={}",
                  report.security_id,
                  report.client_id,
                  report.client_order_id,
                  magic_enum::enum_name(report.reason));
            },
            [this](report::CancellationRejected report) {
              INFO(
                  "Received report, type=CancellationRejected"
                  " client_id={} client_order_id={} client_cancellation_id={} security_id={} reason={}",
                  report.client_id,
                  report.client_order_id,
                  report.client_cancellation_id,
                  report.security_id,
                  magic_enum::enum_name(report.reason));
            },
            [this](report::ModificationAccepted report) {
              INFO(
                  "Received report, type=ModificationAccepted"
                  " order_id={} new_order_id={} client_id={} client_order_id={} client_modification_id={} "
                  "security_id={}"
                  " direction={} price={} quantity={}",
                  report.order_id,
                  report.new_order_id,
                  report.client_id,
                  report.client_order_id,
                  report.client_modification_id,
                  report.security_id,
                  format_as(report.direction),
                  static_cast<double>(report.price),
                  static_cast<double>(report.quantity));
            },
            [this](report::ModificationRejected report) {
              INFO(
                  "Received report, type=ModificationRejected"
                  " client_id={} client_order_id={} client_modification_id={} security_id={} reason={}",
                  report.client_id,
                  report.client_order_id,
                  report.client_modification_id,
                  report.security_id,
                  magic_enum::enum_name(report.reason));
            }),
        report);
  }

private:
};

}  // namespace exchange_sim
