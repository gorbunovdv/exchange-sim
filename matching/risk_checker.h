#pragma once

#include "domain/exchange_snapshots.h"
#include "domain/security.h"
#include "domain/requests.h"
#include <optional>

namespace exchange_sim {

class RiskChecker {
public:
  explicit RiskChecker(const Security& security): security_(security) {}

  std::optional<exchange_snapshot::OrderRejectedReason> reject_request(const request::OrderAddRequest& request) const;

  std::optional<exchange_snapshot::CancellationRejectedReason> reject_request(
      const request::OrderModifyRequest& request) const;

private:
  const Security& security_;
};

}  // namespace exchange_sim
