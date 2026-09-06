#pragma once

#include "domain/security_storage.h"

#include "exchange/domain/reports.h"

namespace exchange_sim::generator_interface_skills {

class PendingTransactions {
public:
  explicit PendingTransactions(const SecurityStorage&) {}

  size_t pending_transactions() const { return pending_transactions_count_; }

  void handle_request(const auto&) { ++pending_transactions_count_; }

  void handle_report(const report::OrderRejected&) { --pending_transactions_count_; }

  void handle_report(const report::OrderBookEntry&) { --pending_transactions_count_; }

  void handle_report(const report::OrderExpired&) { --pending_transactions_count_; }

  void handle_report(const report::OrderFilled& report) {
    if (report.type == engine_events::FilledType::AGGRESSIVE_FILL) {
      --pending_transactions_count_;
    }
  }

  void handle_report(const report::OrderCancelled&) { --pending_transactions_count_; }

  void handle_report(const report::CancellationRejected&) { --pending_transactions_count_; }

  void handle_report(const report::ReplacementRejected&) { --pending_transactions_count_; }

  void handle_report(const auto&) {}
  void handle_market_event(const auto&) {}

  void reserve(size_t) {}

private:
  size_t pending_transactions_count_ = 0;
};

}  // namespace exchange_sim::generator_interface_skills
