#include "store_orders_skill.h"

namespace exchange_sim::generator_interface_skills {

size_t StoreOrders::pending_transactions() const {
  return std::ranges::fold_left(order_storage_, size_t{0}, [](auto current, const auto& order_storage) {
    return current + order_storage.pending_transactions();
  });
}

template<typename RequestType>
void StoreOrders::handle_request(const RequestType& request) {
  if (security_storage_.has_security(request.security_id)) {
    order_storage_[request.security_id].handle(request);
  }
}

template<typename ReportType>
void StoreOrders::handle_report(const ReportType& report) {
  if constexpr (!std::is_same_v<std::remove_cvref_t<decltype(report)>, report::Trade>) {
    if (security_storage_.has_security(report.security_id)) {
      order_storage_[report.security_id].handle(report);
    }
  }
}

template void StoreOrders::handle_request<request::OrderAddRequest>(const request::OrderAddRequest&);
template void StoreOrders::handle_request<request::OrderReplaceRequest>(const request::OrderReplaceRequest&);
template void StoreOrders::handle_request<request::OrderCancelRequest>(const request::OrderCancelRequest&);

template void StoreOrders::handle_report<report::OrderAccepted>(const report::OrderAccepted&);
template void StoreOrders::handle_report<report::Trade>(const report::Trade&);
template void StoreOrders::handle_report<report::OrderBookEntry>(const report::OrderBookEntry&);
template void StoreOrders::handle_report<report::OrderFilled>(const report::OrderFilled&);
template void StoreOrders::handle_report<report::OrderExpired>(const report::OrderExpired&);
template void StoreOrders::handle_report<report::OrderCancelled>(const report::OrderCancelled&);
template void StoreOrders::handle_report<report::OrderRejected>(const report::OrderRejected&);
template void StoreOrders::handle_report<report::CancellationRejected>(const report::CancellationRejected&);
template void StoreOrders::handle_report<report::ReplacementAccepted>(const report::ReplacementAccepted&);
template void StoreOrders::handle_report<report::ReplacementRejected>(const report::ReplacementRejected&);

}  // namespace exchange_sim::generator_interface_skills
