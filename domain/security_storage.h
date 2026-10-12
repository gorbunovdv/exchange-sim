#pragma once

#include "base/logging/loggable_class.h"

#include "domain/security.h"

#include <unordered_map>
#include <vector>

namespace exchange_sim {

class SecurityStorage : public LoggableClass {
public:
  explicit SecurityStorage(const std::vector<SecurityConfig>& securities);

  bool has_security(SecurityID id) const { return id.value() < securities_.size(); }

  std::optional<SecurityID> get_id_by_symbol_or_null(Symbol symbol) const;

  const Security& get_security(SecurityID id) const { return securities_[id.value()]; }

  size_t size() const { return securities_.size(); }
  const std::vector<Security>& securities() const { return securities_; }

  size_t sections_count() const { return sections_count_; }

private:
  void validate_security(Security security) const;

  std::vector<Security> securities_;
  std::unordered_map<Symbol, SecurityID> id_by_symbol_;

  size_t sections_count_;
};


}  // namespace exchange_sim
