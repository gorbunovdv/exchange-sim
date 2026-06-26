#pragma once

#include <base/noncopyable.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace exchange_sim {

inline std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::in);
  return (std::ostringstream() << file.rdbuf()).str();
}

}  // namespace exchange_sim