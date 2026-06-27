#include "config.h"

#include "base/read_file.h"
#include <glaze/glaze.hpp>
#include <filesystem>
#include <stdexcept>
#include <format>

namespace exchange_sim {

glz::generic load_config_from_file(const std::filesystem::path& path) {
  glz::generic result;
  if (auto ec = glz::read<glz::opts{.comments = true}>(result, read_file(path))) {
    throw std::runtime_error(
        std::format("could not read config file, path={}, error={}", path.string(), glz::format_error(ec)));
  }
  return result;
}

}  // namespace exchange_sim