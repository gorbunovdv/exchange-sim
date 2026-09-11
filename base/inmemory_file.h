#pragma once

#include "base/file.h"

#include <filesystem>

namespace exchange_sim {

class InMemoryFile {
public:
  enum class Mode { READ, WRITE };

  explicit InMemoryFile(const std::filesystem::path& path, Mode mode, bool binary = false);

  size_t read(char* buffer, size_t size);

  bool eof() const { return position_ == buffer_.size(); }

private:
  File file_;
  std::vector<char> buffer_;
  size_t position_ = 0;
};

}  // namespace exchange_sim
