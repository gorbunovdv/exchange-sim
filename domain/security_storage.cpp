#include "security_storage.h"

#include "base/logging/logging_backend.h"

namespace exchange_sim {

SecurityStorage::SecurityStorage(const std::vector<SecurityConfig>& securities) {
  securities_.reserve(securities.size());
  for (SecurityConfig security : securities) {
    securities_.push_back(Security{
        .security_id = static_cast<SecurityID>(id_by_symbol_.size()),
        .symbol = security.symbol,
        .tick_size = security.tick_size,
        .lot_size = security.lot_size,
        .low_price = security.low_price,
        .high_price = security.high_price,
        .max_amount = security.max_amount,
    });
    id_by_symbol_.emplace(security.symbol, id_by_symbol_.size());
  }
  CHECK(size() > 0, "Security storage is empty");
  CHECK(id_by_symbol_.size() == size(), "Security storage is not unique");
}

std::optional<SecurityID> SecurityStorage::get_id_by_symbol_or_null(Symbol symbol) const {
  if (auto it = id_by_symbol_.find(symbol); it != id_by_symbol_.end()) {
    return it->second;
  }
  return std::nullopt;
}

}  // namespace exchange_sim