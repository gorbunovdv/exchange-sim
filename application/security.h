#pragma once

#include "application/types.h"
#include <boost/static_string.hpp>

namespace exchange_sim {

using Symbol = boost::static_string<8>;
using SecurityID = uint32_t;

struct Security {
  Symbol symbol;
  Price tick_size;
  Amount lot_size;
  Price low_price;
  Price high_price;
  uint32_t max_amount;
};

class SecurityStorage {
public:
  explicit SecurityStorage(std::vector<Security> securities): securities_(std::move(securities)) {
    for (const auto& security : securities_) {
      id_by_symbol_.emplace(security.symbol, id_by_symbol_.size());
    }
  }

  std::optional<SecurityID> get_id_by_symbol_or_null(Symbol symbol) const {
    if (auto it = id_by_symbol_.find(symbol); it != id_by_symbol_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  const Security& get_security(SecurityID id) const { return securities_[id]; }

  size_t size() const { return securities_.size(); }
  const std::vector<Security>& securities() const { return securities_; }

private:
  std::vector<Security> securities_;
  std::unordered_map<Symbol, SecurityID> id_by_symbol_;
};

}  // namespace exchange_sim
