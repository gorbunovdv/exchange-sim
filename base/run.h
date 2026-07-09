#pragma once

#include "base/threading/core_allocator.h"

#include <glaze/glaze.hpp>

namespace exchange_sim {

std::tuple<CoreLease, std::shared_ptr<CoreAllocator>, glz::generic> read_config_and_initialize_logging(
    std::span<char*> args);

}  // namespace exchange_sim