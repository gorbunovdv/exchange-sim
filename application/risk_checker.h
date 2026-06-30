#pragma once
#include "domain/exchange_snapshots.h"
#include "domain/security.h"

namespace exchange_sim {

class RiskChecker {
public:
  explicit RiskChecker(const Security& security): security_(security) {}

  std::optional<exchange_snapshot::OrderRejectedReason> reject_request(const request::OrderAddRequest& request) const {
    if (request.quantity <= 0 || request.quantity > security_.max_amount) {
      return exchange_snapshot::OrderRejectedReason::INSUFFICIENT_FUNDS;
    }
    if (request.price < security_.low_price || request.price > security_.high_price) {
      return exchange_snapshot::OrderRejectedReason::PRICE_OUT_OF_RANGE;
    }
    return std::nullopt;
  }

  std::optional<exchange_snapshot::CancellationRejectedReason> reject_request(
      const request::OrderModifyRequest& request) const {
    if (request.quantity <= 0 || request.quantity > security_.max_amount) {
      return exchange_snapshot::CancellationRejectedReason::INSUFFICIENT_FUNDS;
    }
    if (request.price < security_.low_price || request.price > security_.high_price) {
      return exchange_snapshot::CancellationRejectedReason::PRICE_OUT_OF_RANGE;
    }
    return std::nullopt;
  }

private:
  const Security& security_;
};

}  // namespace exchange_sim
