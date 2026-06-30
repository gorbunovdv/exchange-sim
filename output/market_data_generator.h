#pragma once

#include "base/logging/logging_backend.h"
#include "domain/types.h"
#include "domain/market_data_snapshots.h"
#include <boost/hof/match.hpp>
#include <glaze/glaze.hpp>

namespace exchange_sim {

class MarketDataGenerator {
public:
  explicit MarketDataGenerator(const glz::generic&) {}

  void process(market_data::SnapshotType snapshot) {
    std::visit(boost::hof::match(
                   [](market_data::OrderEntrySnapshot snapshot) {
                     INFO(
                         "Received market data snapshot, type=OrderEntrySnapshot"
                         " order_id={} security_id={}"
                         " direction={} price={} quantity={}",
                         snapshot.order_id,
                         snapshot.security_id,
                         format_as(snapshot.direction),
                         static_cast<double>(snapshot.price),
                         static_cast<double>(snapshot.quantity));
                   },
                   [](market_data::OrderCancellationSnapshot snapshot) {
                     INFO(
                         "Received market data snapshot, type=OrderCancellationSnapshot"
                         " order_id={} security_id={}",
                         snapshot.order_id,
                         snapshot.security_id);
                   },
                   [](market_data::TradeSnapshot snapshot) {
                     INFO(
                         "Received market data snapshot, type=TradeSnapshot"
                         " security_id={} price={} quantity={}"
                         " trade_id={} aggressor_order_id={} recipient_order_id={}",
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