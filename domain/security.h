#pragma once

#include "domain/types.h"
#include <boost/static_string.hpp>

namespace exchange_sim {

using Symbol = boost::static_string<8>;
using SecurityID = uint32_t;

struct SecurityConfig {
  Symbol symbol;
  Price tick_size;
  Amount lot_size;
  Price low_price;
  Price high_price;
  Amount max_amount;
};

struct Security {
  SecurityID security_id;
  Symbol symbol;
  Price tick_size;
  Amount lot_size;
  Price low_price;
  Price high_price;
  Amount max_amount;
};

}  // namespace exchange_sim
