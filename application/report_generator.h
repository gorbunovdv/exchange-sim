#pragma once

#include "base/logging/loggable_class.h"
#include "application/reports.h"

#include <boost/hof/match.hpp>
#include <glaze/json/generic_fwd.hpp>

namespace exchange_sim {

class ReportGenerator : public LoggableClass {
public:
  explicit ReportGenerator(const glz::generic&): LoggableClass("report_generator") {}

  void process(OrderReportType report) {
    std::visit(boost::hof::match(
                   [this](const OrderEntryReport& report) {
                     INFO("Received report, type=OrderEntryReport order_id={} client_order_id={} outcome={}",
                         report.order_id,
                         report.client_order_id,
                         format_as(report.outcome));
                   },
                   [this](const OrderCancellationReport& report) {
                     INFO("Received report, type=OrderEntryReport order_id={} client_order_id={}",
                         report.order_id,
                         report.client_order_id);
                   },
                   [this](const OrderTradeReport& report) {
                     INFO(
                         "Received report, type=OrderEntryReport order_id={} client_order_id={} price={} quantity={} "
                         "trade_id={} aggressor_order_id={} recipient_order_id={}",
                         report.order_id,
                         report.client_order_id,
                         static_cast<double>(report.price),
                         static_cast<double>(report.quantity),
                         report.trade_id,
                         report.aggressor_order_id,
                         report.recipient_order_id);
                   }),
        report);
  }

private:
};

}  // namespace exchange_sim
