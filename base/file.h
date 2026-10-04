#pragma once

#include "logging/logging_backend.h"

#include <filesystem>
#include <fstream>
#include <span>

namespace exchange_sim {

class File : NonCopyable {
public:
  enum class Mode : uint8_t { READ, WRITE };

  explicit File(const std::filesystem::path& path, Mode mode, bool binary = false);

  size_t read(std::span<char> buffer);

  size_t write(std::span<const char> buffer);

  bool eof() const { return file_.eof(); }

private:
  std::fstream file_;
};

}  // namespace exchange_sim
