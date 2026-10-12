#pragma once

#include "base/strong_type.h"

#include "domain/types.h"

#include <boost/static_string.hpp>

namespace exchange_sim {

using SecurityID = StrongType<uint32_t, struct SecurityIDTag, skills::EqualTo>;
using Symbol = StrongType<boost::static_string<8>, struct SymbolTag, skills::EqualTo, skills::Hashable>;
using SectionID = StrongType<uint32_t, struct SectionIDTag, skills::EqualTo>;

struct SecurityConfig {
  Symbol symbol;
  Price tick_size;
  Amount lot_size;
  Price low_price;
  Price high_price;
  Amount max_amount;
  std::string section;
};

struct Security {
  SecurityID security_id;
  Symbol symbol;
  Price tick_size;
  Amount lot_size;
  Price low_price;
  Price high_price;
  Amount max_amount;
  SectionID section_id;
};

}  // namespace exchange_sim
