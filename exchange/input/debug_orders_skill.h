#pragma once

#include "domain/security_storage.h"

#include "exchange/domain/market_events.h"
#include "exchange/domain/reports.h"
#include "exchange/domain/requests.h"

namespace exchange_sim::generator_interface_skills {

class DebugOrders {
public:
  explicit DebugOrders(const SecurityStorage&) {}

  void handle_request(const request::OrderAddRequest& request);
  void handle_request(const request::OrderReplaceRequest& request);
  void handle_request(const request::OrderCancelRequest& request);

  void handle_market_event(const market_events::BookOrderEntry& snapshot);
  void handle_market_event(const market_events::BookOrderChange& snapshot);
  void handle_market_event(const market_events::BookOrderDeletion& snapshot);
  void handle_market_event(const market_events::Trade& snapshot);

  void handle_report(const report::OrderAccepted& report);
  void handle_report(const report::Trade& report);
  void handle_report(const report::OrderBookEntry& report);
  void handle_report(const report::OrderFilled& report);
  void handle_report(const report::OrderExpired& report);
  void handle_report(const report::OrderCancelled& report);
  void handle_report(const report::OrderRejected& report);
  void handle_report(const report::CancellationRejected& report);
  void handle_report(const report::ReplacementAccepted& report);
  void handle_report(const report::ReplacementRejected& report);
};

}  // namespace exchange_sim::generator_interface_skills
