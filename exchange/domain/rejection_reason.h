#pragma once

#include "exchange/domain/engine_events.h"
#include "exchange/domain/requests.h"

namespace exchange_sim {

template<typename>
struct RejectionReason {};

template<>
struct RejectionReason<request::OrderAddRequest> {
  using ReasonType = engine_events::OrderRejectedReason;
};

template<>
struct RejectionReason<request::OrderReplaceRequest> {
  using ReasonType = engine_events::ReplacementRejectedReason;
};

}  // namespace exchange_sim