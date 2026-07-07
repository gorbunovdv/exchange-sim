#pragma once

#include <glaze/json/generic_fwd.hpp>

#include <filesystem>

namespace exchange_sim {

glz::generic load_config_from_file(const std::filesystem::path& path);

}  // namespace exchange_sim