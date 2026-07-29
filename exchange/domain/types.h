#pragma once

#include "base/strong_type.h"

#include <cstdint>

namespace exchange_sim {

using ClientID = StrongType<uint32_t, struct ClientIDTag, skills::EqualTo, skills::Hashable>;
using ClientOrderID = StrongType<uint32_t, struct ClientOrderIDTag, skills::EqualTo, skills::Hashable>;
using ExchangeOrderID = StrongType<uint32_t, struct ExchangeOrderIDTag, skills::EqualTo>;
using ExchangeTradeID = StrongType<uint32_t, struct ExchangeTradeIDTag, skills::EqualTo>;

}  // namespace exchange_sim
