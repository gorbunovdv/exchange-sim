#pragma once

#include "base/strong_type.h"

#include <cstdint>

namespace exchange_sim {

using ClientID = StrongType<uint32_t, struct ClientIDTag, skills::EqualTo, skills::Hashable>;
using ClientOrderID = StrongType<uint32_t, struct ClientOrderIDTag, skills::EqualTo, skills::Hashable>;
using ExchangeOrderID = StrongType<uint32_t, struct ExchangeOrderIDTag, skills::EqualTo, skills::Hashable>;
using ExchangeTradeID = StrongType<uint32_t, struct ExchangeTradeIDTag, skills::EqualTo>;

}  // namespace exchange_sim

template<>
struct std::hash<std::pair<exchange_sim::ClientID, exchange_sim::ClientOrderID>> {
  size_t operator()(const std::pair<exchange_sim::ClientID, exchange_sim::ClientOrderID>& pair) const noexcept {
    return std::hash<uint64_t>{}(((std::hash<exchange_sim::ClientID>{}(pair.first) << 32) |
        std::hash<exchange_sim::ClientOrderID>{}(pair.second)));
  }
};
