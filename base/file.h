#pragma once

#include "logging/logging_backend.h"

#include <magic_enum/magic_enum.hpp>

#include <filesystem>
#include <fstream>

namespace exchange_sim {

class File {
public:
  enum class Mode { READ, WRITE };

  explicit File(const std::filesystem::path& path, Mode mode, bool binary = false);

  size_t read(char* buffer, size_t size);

  size_t write(const char* buffer, size_t size);

  bool eof() const { return file_.eof(); }

private:
  std::fstream file_;
};

}  // namespace exchange_sim
