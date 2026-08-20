#pragma once

#include "domain/security.h"

#include <glaze/glaze.hpp>

#include <vector>

namespace exchange_sim {

std::vector<SecurityConfig> generate_securities(const glz::generic& config);

}  // namespace exchange_sim