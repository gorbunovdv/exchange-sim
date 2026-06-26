#pragma once

#include "base/logging/tests/helper.h"

#include <glaze/glaze.hpp>

namespace exchange_sim {

inline glz::generic load_config_from_file(const std::filesystem::path& path) {
  glz::generic result;
  if (auto ec = glz::read<glz::opts{.comments = true}>(result, read_file(path))) {
    throw std::runtime_error(
        std::format("could not read config file, path={}, error={}", path.string(), glz::format_error(ec)));
  }
  return result;
}

}  // namespace exchange_sim