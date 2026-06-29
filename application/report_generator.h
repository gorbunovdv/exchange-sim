#pragma once

#include "base/logging/loggable_class.h"
#include "application/reports.h"

#include <boost/hof/match.hpp>
#include <glaze/json/generic_fwd.hpp>

namespace exchange_sim {

class ReportGenerator : public LoggableClass {
public:
  explicit ReportGenerator(const glz::generic&): LoggableClass("report_generator") {}

  void process(report::ReportType report) {
    std::visit(boost::hof::match(
                   [this](const report::OrderEntryReport& report) {
                     INFO(
                         "Received report, type=OrderEntryReport"
                         " client_id={} order_id={} client_order_id={}",
                         report.client_id,
                         report.order_id,
                         report.client_order_id);
                   },
                   [this](const report::OrderCancellationReport& report) {
                     INFO(
                         "Received report, type=OrderCancellationReport"
                         " client_id={} order_id={} client_order_id={}",
                         report.client_id,
                         report.order_id,
                         report.client_order_id);
                   },
                   [this](const report::OrderTradeReport& report) {
                     INFO(
                         "Received report, type=OrderTradeReport"
                         " order_id={} client_id={} client_order_id={} price={} quantity={}"
                         " trade_id={} aggressor_order_id={} recipient_order_id={}",
                         report.order_id,
                         report.client_id,
                         report.client_order_id,
                         static_cast<double>(report.price),
                         static_cast<double>(report.quantity),
                         report.trade_id,
                         report.aggressor_order_id,
                         report.recipient_order_id);
                   },
                   [this](const report::OrderRejectedReport& report) {
                     INFO(
                         "Received report, type=OrderRejectedReport"
                         " security_id={} client_id={} client_order_id={} reason={}",
                         report.security_id,
                         report.client_id,
                         report.client_order_id,
                         magic_enum::enum_name<report::OrderRejectedReason>(report.reason));
                   },
                   [this](const report::CancellationRejectedReport& report) {
                     INFO(
                         "Received report, type=CancellationRejectedReport"
                         " security_id={} client_id={} client_order_id={} reason={}",
                         report.security_id,
                         report.client_id,
                         report.client_order_id,
                         magic_enum::enum_name<report::CancellationRejectedReason>(report.reason));
                   }),
        report);
  }

private:
};

}  // namespace exchange_sim
