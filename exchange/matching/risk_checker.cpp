#include "risk_checker.h"

namespace exchange_sim {

template<typename Request>
std::optional<typename RejectionReason<Request>::ReasonType> RiskChecker::reject_request(const Request& request) const {
  if (request.quantity > security_.max_amount) {
    return RejectionReason<Request>::ReasonType::MAX_AMOUNT_EXCEEDED;
  }
  if (request.quantity <= 0 || request.quantity % security_.lot_size != 0) {
    return RejectionReason<Request>::ReasonType::INVALID_QUANTITY;
  }
  if (request.price < security_.low_price || request.price > security_.high_price) {
    return RejectionReason<Request>::ReasonType::PRICE_OUT_OF_RANGE;
  }
  if (request.price % security_.tick_size != 0) {
    return RejectionReason<Request>::ReasonType::PRICE_NOT_MULTIPLE_OF_TICK_SIZE;
  }
  return std::nullopt;
}

template std::optional<RejectionReason<request::OrderAddRequest>::ReasonType> RiskChecker::reject_request(
    const request::OrderAddRequest& request) const;

template std::optional<RejectionReason<request::OrderReplaceRequest>::ReasonType> RiskChecker::reject_request(
    const request::OrderReplaceRequest& request) const;

}  // namespace exchange_sim
