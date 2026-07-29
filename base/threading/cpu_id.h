#pragma once

#include "base/strong_type.h"

#include <cstdint>

namespace exchange_sim {

using CpuID = StrongType<uint32_t, struct CpuIDType, skills::EqualTo>;

}  // namespace exchange_sim
