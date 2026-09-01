#pragma once

#include "base/strong_type.h"

#include "domain/security_storage.h"

#include "exchange/input/order_storage.h"

namespace exchange_sim::generator_interface_skills {

class StoreOrders {
public:
  using OrderStorage = exchange_sim::OrderStorage;

  explicit StoreOrders(const SecurityStorage& security_storage):
      security_storage_(security_storage), order_storage_(security_storage_.size()) {}

  const auto& order_storage() const { return order_storage_; }

  size_t pending_transactions() const;

  template<typename RequestType>
  void handle_request(const RequestType& request);

  template<typename ReportType>
  void handle_report(const ReportType& report);

  void handle_market_event(const auto&) {}

  void reserve(size_t size) {
    for (auto& storage : order_storage_) {
      storage.reserve(size);
    }
  }

private:
  const SecurityStorage& security_storage_;
  StrongTypeIndexedVector<SecurityID, OrderStorage> order_storage_;
};

}  // namespace exchange_sim::generator_interface_skills