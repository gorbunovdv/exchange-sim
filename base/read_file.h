#pragma once

#include <fstream>
#include <string>

namespace exchange_sim {

inline std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::in);
  if (!file) {
    throw std::runtime_error(std::format("could not open file, path={}", path.string()));
  }
  return std::string(std::istreambuf_iterator(file), std::istreambuf_iterator<char>());
}

}  // namespace exchange_sim