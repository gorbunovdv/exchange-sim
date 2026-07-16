#pragma once

#include "base/threading/core_allocator.h"
#include "base/threading/runtime.h"

#include <glaze/glaze.hpp>

namespace exchange_sim {

std::tuple<std::shared_ptr<Runtime>, glz::generic> read_config_and_initialize_logging(std::span<char*> args);

}  // namespace exchange_sim