#include "security_storage.h"

#include "base/logging/logging_backend.h"

namespace exchange_sim {

SecurityStorage::SecurityStorage(const std::vector<SecurityConfig>& securities): LoggableClass("SecurityStorage") {
  securities_.reserve(securities.size());
  for (SecurityConfig security_config : securities) {
    Security security{
        .security_id = static_cast<SecurityID>(id_by_symbol_.size()),
        .symbol = security_config.symbol,
        .tick_size = security_config.tick_size,
        .lot_size = security_config.lot_size,
        .low_price = security_config.low_price,
        .high_price = security_config.high_price,
        .max_amount = security_config.max_amount,
    };
    validate_security(security);
    securities_.push_back(security);
    id_by_symbol_.emplace(security_config.symbol, id_by_symbol_.size());
    INFO("Added security, symbol={} lot_size={} tick_size={} low_price={} high_price={} max_amount={}",
        std::string(security.symbol.value()),
        static_cast<double>(security.lot_size.value()),
        static_cast<double>(security.tick_size.value()),
        static_cast<double>(security.low_price.value()),
        static_cast<double>(security.high_price.value()),
        static_cast<double>(security.max_amount.value()));
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

void SecurityStorage::validate_security(Security security) const {
  CHECK(security.tick_size > Price{0}, "Tick size must be positive");
  CHECK((security.high_price - security.low_price) % security.tick_size == Price{0},
      "Price range must be multiple of tick size");
  CHECK(security.high_price > security.low_price, "High price must be greater than low price");

  CHECK(security.lot_size > Amount{0}, "Lot size must be positive");
  CHECK(security.max_amount > Amount{0}, "Max amount must be positive");
  CHECK(security.max_amount % security.lot_size == Amount{0}, "Max amount must be multiple of lot size");
}

}  // namespace exchange_sim