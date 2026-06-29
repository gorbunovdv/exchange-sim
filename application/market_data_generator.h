#pragma once

#include "base/logging/logging_backend.h"
#include "application/types.h"
#include "application/market_data_snapshots.h"
#include <boost/hof/match.hpp>

namespace exchange_sim {

class MarketDataGenerator {
public:
  explicit MarketDataGenerator(const glz::generic&) {}

  void process(market_data::SnapshotType snapshot) {
    std::visit(boost::hof::match(
                   [](const market_data::OrderEntrySnapshot& snapshot) {
                     INFO(
                         "Received snapshot, type=OrderEntrySnapshot"
                         " order_id={} security_id={} time_in_force={}"
                         " order_type={} direction={} price={} quantity={}",
                         snapshot.order_id,
                         snapshot.security_id,
                         format_as(snapshot.time_in_force),
                         format_as(snapshot.order_type),
                         format_as(snapshot.direction),
                         static_cast<double>(snapshot.price),
                         static_cast<double>(snapshot.quantity));
                   },
                   [](const market_data::OrderCancellationSnapshot& snapshot) {
                     INFO(
                         "Received snapshot, type=OrderCancellationSnapshot"
                         " order_id={} security_id={}",
                         snapshot.order_id,
                         snapshot.security_id);
                   },
                   [](const market_data::TradeSnapshot& snapshot) {
                     INFO(
                         "Received snapshot, type=TradeSnapshot"
                         " security_id={} price={} quantity={} trade_id={}"
                         " aggressor_order_id={} recipient_order_id={}",
                         snapshot.security_id,
                         static_cast<double>(snapshot.price),
                         static_cast<double>(snapshot.quantity),
                         snapshot.trade_id,
                         snapshot.aggressor_order_id,
                         snapshot.recipient_order_id);
                   }),
        snapshot);
  }

private:
};

}  // namespace exchange_sim