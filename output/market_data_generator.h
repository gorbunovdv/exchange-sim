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

  void process(market_data_snapshot::SnapshotType snapshot) {
    std::visit(boost::hof::match(
                   [](market_data_snapshot::BookOrderEntry snapshot) {
                     INFO(
                         "Received market data snapshot, type=BookOrderEntry"
                         " order_id={} security_id={}"
                         " direction={} price={} quantity={}",
                         snapshot.order_id,
                         snapshot.security_id,
                         format_as(snapshot.direction),
                         static_cast<double>(snapshot.price),
                         static_cast<double>(snapshot.quantity));
                   },
                   [](market_data_snapshot::BookOrderChange snapshot) {
                     INFO(
                         "Received market data snapshot, type=BookOrderChange"
                         " order_id={} security_id={} direction={} price={} new_quantity={}",
                         snapshot.order_id,
                         snapshot.security_id,
                         format_as(snapshot.direction),
                         static_cast<double>(snapshot.price),
                         static_cast<double>(snapshot.new_quantity));
                   },
                   [](market_data_snapshot::BookOrderDeletion snapshot) {
                     INFO(
                         "Received market data snapshot, type=BookOrderDeletion"
                         " order_id={} security_id={}",
                         snapshot.order_id,
                         snapshot.security_id);
                   },
                   [](market_data_snapshot::Trade snapshot) {
                     INFO(
                         "Received market data snapshot, type=Trade"
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