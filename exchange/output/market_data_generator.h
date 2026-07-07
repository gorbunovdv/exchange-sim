#pragma once

#include "base/logging/logging_backend.h"

#include "domain/types.h"

#include "exchange/domain/market_events.h"

#include <boost/hof/match.hpp>
#include <glaze/glaze.hpp>

namespace exchange_sim {

class MarketDataGenerator {
public:
  explicit MarketDataGenerator(const glz::generic&) { static_assert(market_events::Processor<MarketDataGenerator>); }

  void process(market_events::EventType&& event) {
    std::visit(boost::hof::match(
                   [](market_events::BookOrderEntry&& event) {
                     INFO(
                         "Received market data event, type=BookOrderEntry"
                         " order_id={} security_id={}"
                         " direction={} price={} quantity={}",
                         event.order_id,
                         event.security_id.value(),
                         format_as(event.direction),
                         static_cast<double>(event.price),
                         static_cast<double>(event.quantity));
                   },
                   [](market_events::BookOrderChange&& event) {
                     INFO(
                         "Received market data event, type=BookOrderChange"
                         " order_id={} security_id={} direction={} price={} new_quantity={}",
                         event.order_id,
                         event.security_id.value(),
                         format_as(event.direction),
                         static_cast<double>(event.price),
                         static_cast<double>(event.new_quantity));
                   },
                   [](market_events::BookOrderDeletion&& event) {
                     INFO(
                         "Received market data event, type=BookOrderDeletion"
                         " order_id={} security_id={}",
                         event.order_id,
                         event.security_id.value());
                   },
                   [](market_events::Trade&& event) {
                     INFO(
                         "Received market data event, type=Trade"
                         " security_id={} price={} quantity={}"
                         " trade_id={} aggressor_order_id={} recipient_order_id={}",
                         event.security_id.value(),
                         static_cast<double>(event.price),
                         static_cast<double>(event.quantity),
                         event.trade_id,
                         event.aggressor_order_id,
                         event.recipient_order_id);
                   }),
        std::move(event));
  }

private:
};

}  // namespace exchange_sim