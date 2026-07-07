#pragma once

#include "domain/security.h"

#include "exchange/domain/engine_events.h"
#include "exchange/domain/rejection_reason.h"
#include "exchange/domain/requests.h"

#include <optional>

namespace exchange_sim {

class RiskChecker {
public:
  explicit RiskChecker(const Security& security): security_(security) {}

  template<typename Request>
  std::optional<typename RejectionReason<Request>::ReasonType> reject_request(const Request& request) const;

private:
  const Security& security_;
};

}  // namespace exchange_sim
