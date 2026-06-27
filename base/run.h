#pragma once

#include <glaze/glaze.hpp>

namespace exchange_sim {

glz::generic read_config_and_initialize_logging(std::span<char*> args);

}  // namespace exchange_sim