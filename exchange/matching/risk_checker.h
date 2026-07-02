#pragma once

#include "exchange/domain/engine_snapshots.h"
#include "domain/security.h"
#include "exchange/domain/requests.h"
#include <optional>

namespace exchange_sim {

class RiskChecker {
public:
  explicit RiskChecker(const Security& security): security_(security) {}

  std::optional<engine_snapshot::OrderRejectedReason> reject_request(const request::OrderAddRequest& request) const;

  std::optional<engine_snapshot::ReplacementRejectedReason> reject_request(
      const request::OrderReplaceRequest& request) const;

private:
  const Security& security_;
};

}  // namespace exchange_sim
