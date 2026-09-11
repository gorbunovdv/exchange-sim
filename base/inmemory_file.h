#pragma once

#include "base/file.h"

#include <filesystem>

namespace exchange_sim {

class InMemoryFile {
public:
  enum class Mode : uint8_t { READ, WRITE };

  explicit InMemoryFile(const std::filesystem::path& path, Mode mode, bool binary = false);

  size_t read(std::span<char> buffer);

  bool eof() const { return position_ == buffer_.size(); }

private:
  File file_;
  std::vector<char> buffer_;
  size_t position_ = 0;
};

}  // namespace exchange_sim
